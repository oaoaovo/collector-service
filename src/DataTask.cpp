#include "DataTask.h"

#include "DriverClient.h"
#include "ModelParser.h"
#include "SQLiteManager.h"
#include "task/CollectionQueryBuilder.h"
#include "task/CollectionResponseParser.h"
#include "util/Logger.h"
#include "util/TimeUtil.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {

constexpr int kMaxRowsPerDevice = 5000;
constexpr int kCleanupIntervalMs = 60000;

} // namespace

DataTask::DataTask(SQLiteManager& sqliteManager) : sqliteManager_(sqliteManager) {}

DataTask::DataTask(SQLiteManager& sqliteManager, ModelRepository modelRepository)
    : sqliteManager_(sqliteManager), modelRepository_(std::move(modelRepository)) {}

DataTask::~DataTask() {
    stopAll();
}

void DataTask::startAll() {
    const auto devices = sqliteManager_.getDevices();
    int startedCount = 0;
    int skippedCount = 0;

    for (const auto& device : devices) {
        if (device.status == 0) {
            ++skippedCount;
            Logger::warn("DataTask startAll skipped offline device: " + device.name);
            continue;
        }

        const auto result = startDeviceCollect(device.name, "");
        if (result.ok()) {
            ++startedCount;
        } else {
            ++skippedCount;
            Logger::warn("DataTask startAll skipped " + device.name + ": " + result.msg);
        }
    }

    Logger::info("DataTask startAll finished, started=" + std::to_string(startedCount) +
                 ", skipped=" + std::to_string(skippedCount));
}

void DataTask::stopAll() {
    statusStore_.requestStopAll();
    auto threads = statusStore_.takeAllThreads();

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    statusStore_.clearAllWorkers();
    Logger::info("DataTask stopped");
}

std::vector<DeviceStatus> DataTask::getStatus() const {
    return statusStore_.getStatus();
}

CommandResult DataTask::initDataPointsByDevice(const std::string& deviceName, const std::string& mid) {
    CommandResult result;
    result.mid = mid;

    try {
        if (!sqliteManager_.deviceExists(deviceName)) {
            throw std::runtime_error("device not found: " + deviceName);
        }

        const auto device = sqliteManager_.getDeviceByName(deviceName);
        ModelParser parser;
        const auto modelText = modelRepository_.loadModelText(device.model, device.modelVersion);
        const auto points = parser.parseModelToDataPoints(device, modelText);
        sqliteManager_.replaceDataPointsForDevice(device, points);
        result.msg = "DataPoint initialized for device: " + deviceName;
        Logger::info(result.msg);
    } catch (const std::exception& ex) {
        result.code = 500;
        result.msg = ex.what();
        Logger::error("initDataPointsByDevice failed: " + result.msg);
    }

    return result;
}

CommandResult DataTask::startDeviceCollect(const std::string& deviceName, const std::string& mid) {
    CommandResult result;
    result.mid = mid;

    try {
        if (!sqliteManager_.deviceExists(deviceName)) {
            throw std::runtime_error("device not found: " + deviceName);
        }
        auto device = sqliteManager_.getDeviceByName(deviceName);
        if (device.status == 0) {
            throw std::runtime_error("device is offline: " + deviceName);
        }

        auto points = sqliteManager_.getDataPointsByDeviceName(deviceName);
        if (points.empty()) {
            const auto initResult = initDataPointsByDevice(deviceName, mid);
            if (!initResult.ok()) {
                throw std::runtime_error(initResult.msg);
            }
            points = sqliteManager_.getDataPointsByDeviceName(deviceName);
        }
        if (points.empty()) {
            throw std::runtime_error("device has no configured data points: " + deviceName);
        }

        if (statusStore_.isActive(deviceName)) {
            throw std::runtime_error("device collection is already running: " + deviceName);
        }

        auto staleThread = statusStore_.takeThread(deviceName);
        statusStore_.eraseStopFlag(deviceName);
        if (staleThread.joinable()) {
            staleThread.join();
        }

        auto stopFlag = std::make_shared<std::atomic_bool>(false);
        statusStore_.addWorker(deviceName, stopFlag, std::thread(&DataTask::runDeviceTask, this, device, stopFlag));
        result.msg = "device collection started: " + deviceName;
    } catch (const std::exception& ex) {
        statusStore_.markStartFailed(deviceName);
        result.code = 500;
        result.msg = ex.what();
    }

    return result;
}

CommandResult DataTask::stopDeviceCollect(const std::string& deviceName, const std::string& mid) {
    CommandResult result;
    result.mid = mid;

    if (!sqliteManager_.deviceExists(deviceName)) {
        result.code = 500;
        result.msg = "device not found: " + deviceName;
        return result;
    }

    if (!statusStore_.requestStop(deviceName)) {
        result.msg = "device collection is not running: " + deviceName;
        return result;
    }

    auto threadToJoin = statusStore_.takeThread(deviceName);
    statusStore_.eraseStopFlag(deviceName);
    if (threadToJoin.joinable()) {
        threadToJoin.join();
    }

    statusStore_.markStopped(deviceName);
    result.msg = "device collection stopped: " + deviceName;
    Logger::info(result.msg);
    return result;
}

void DataTask::runDeviceTask(Device device, std::shared_ptr<std::atomic_bool> stopFlag) {
    DeviceStatus status;
    status.deviceName = device.name;
    int failCount = 0;
    auto lastCleanupTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(kCleanupIntervalMs);

    while (!*stopFlag) {
        std::string stage = "prepare";
        try {
            stage = "loadDevice";
            device = sqliteManager_.getDeviceByName(device.name);
            if (device.status == 0) {
                throw std::runtime_error("device is offline: " + device.name);
            }

            stage = "loadDataPoints";
            auto points = sqliteManager_.getDataPointsByDeviceName(device.name);
            if (points.empty()) {
                stage = "initDataPoints";
                const auto initResult = initDataPointsByDevice(device.name, "");
                if (!initResult.ok()) {
                    throw std::runtime_error(initResult.msg);
                }
                stage = "loadDataPoints";
                points = sqliteManager_.getDataPointsByDeviceName(device.name);
            }
            if (points.empty()) {
                throw std::runtime_error("device has no configured data points: " + device.name);
            }

            stage = "buildQuery";
            const auto cid = task::makeCid(device.name);
            const auto query = task::buildQuery(cid, points);
            Logger::info("DataTask query: " + query);

            stage = "connect";
            DriverClient driverClient(device.ip, device.port);
            status.connected = driverClient.connect();
            if (!status.connected) {
                throw std::runtime_error("DriverClient connect failed for " + device.name +
                                         " at ws://" + device.ip + ":" + std::to_string(device.port));
            }

            stage = "sendQuery";
            const auto response = driverClient.sendQuery(query);
            if (response.empty()) {
                throw std::runtime_error("response is empty");
            }
            Logger::info("DataTask response: " + response);

            stage = "processResponse";
            const auto records = task::processResponse(device, points, response);
            sqliteManager_.insertResources(records);
            Logger::info("DataTask inserted " + std::to_string(records.size()) + " Resources records");

            stage = "close";
            driverClient.close();

            const auto now = std::chrono::steady_clock::now();
            if (now - lastCleanupTime >= std::chrono::milliseconds(kCleanupIntervalMs)) {
                try {
                    sqliteManager_.cleanupResourcesForDevice(device.name, kMaxRowsPerDevice);
                    lastCleanupTime = now;
                    Logger::info("DataTask cleanupResources succeeded for " + device.name +
                                 ", maxRowsPerDevice=" + std::to_string(kMaxRowsPerDevice));
                } catch (const std::exception& ex) {
                    Logger::warn("DataTask cleanupResources failed for " + device.name + ": " + ex.what());
                }
            }

            failCount = 0;
            status.connected = true;
            status.machineConnected = task::extractBoolField(response, "machineConnected");
            status.failCount = 0;
            status.lastSuccessTime = util::nowSeconds();
            status.error.clear();
            statusStore_.setLatestStatus(status);
            Logger::info("DataTask collection succeeded for " + device.name);
        } catch (const std::exception& ex) {
            ++failCount;
            status.connected = false;
            status.machineConnected = false;
            status.failCount = failCount;
            status.error = stage + " failed: " + ex.what();
            statusStore_.setLatestStatus(status);
            Logger::error("DataTask collection failed for " + device.name +
                          ", stage=" + stage + ": " + ex.what());
            if (failCount >= 5) {
                Logger::error("DataTask stopped after 5 consecutive failures for " + device.name);
                break;
            }
        }

        sleepUntilNextCycle(device.sampleIntervalMs, stopFlag);
    }

    statusStore_.markStopped(device.name);
}

void DataTask::sleepUntilNextCycle(int sampleIntervalMs, const std::shared_ptr<std::atomic_bool>& stopFlag) {
    const auto interval = std::chrono::milliseconds(std::max(sampleIntervalMs, 100));
    const auto step = std::chrono::milliseconds(100);
    auto slept = std::chrono::milliseconds(0);
    while (!*stopFlag && slept < interval) {
        const auto remaining = interval - slept;
        const auto currentStep = remaining < step ? remaining : step;
        std::this_thread::sleep_for(currentStep);
        slept += currentStep;
    }
}
