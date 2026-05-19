#include "DataTask.h"

#include "DriverClient.h"
#include "Logger.h"
#include "ModelParser.h"
#include "SQLiteManager.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {

constexpr int kMaxRowsPerDevice = 5000;
constexpr int kCleanupIntervalMs = 60000;

std::string escapeJson(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

//鏋勫缓鏌ヨ鐐逛綅
std::string buildQuery(const std::string& cid, const std::vector<DataPoint>& points) {
    if (points.empty()) {
        throw std::runtime_error("query construct failed: no DataPoint");
    }

    std::ostringstream query;
    query << "{\"cid\":\"" << escapeJson(cid) << "\",\"query\":[";
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i > 0) {
            query << ",";
        }
        query << "\"" << escapeJson(points[i].namePath) << "\"";
    }
    query << "]}";
    return query.str();
}

std::string makeCid(const std::string& deviceName) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return deviceName + "-" + std::to_string(millis);
}

double nowSeconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

std::string extractObjectForPath(const std::string& responseText, const std::string& path) {
    const std::string key = "\"" + path + "\"";
    const auto keyPos = responseText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto objectStart = responseText.find('{', keyPos + key.size());
    if (objectStart == std::string::npos) {
        return {};
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (auto pos = objectStart; pos < responseText.size(); ++pos) {
        const char ch = responseText[pos];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = inString;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (inString) {
            continue;
        }
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return responseText.substr(objectStart, pos - objectStart + 1);
            }
        }
    }

    return {};
}

std::string extractStringField(const std::string& objectText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto colonPos = objectText.find(':', keyPos + key.size());
    const auto quotePos = objectText.find('"', colonPos);
    if (colonPos == std::string::npos || quotePos == std::string::npos) {
        return {};
    }

    std::string value;
    bool escaped = false;
    for (auto pos = quotePos + 1; pos < objectText.size(); ++pos) {
        const char ch = objectText[pos];
        if (escaped) {
            value += ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return value;
        }
        value += ch;
    }
    return {};
}

std::string extractArrayFieldText(const std::string& objectText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto arrayStart = objectText.find('[', keyPos + key.size());
    const auto arrayEnd = objectText.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        return {};
    }
    return objectText.substr(arrayStart, arrayEnd - arrayStart + 1);
}

double extractNumberField(const std::string& objectText, const std::string& fieldName, double fallback) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    const auto colonPos = objectText.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return fallback;
    }

    const auto valueStart = objectText.find_first_of("-0123456789", colonPos + 1);
    if (valueStart == std::string::npos) {
        return fallback;
    }

    const auto valueEnd = objectText.find_first_not_of("-0123456789.eE+", valueStart);
    try {
        return std::stod(objectText.substr(valueStart, valueEnd - valueStart));
    } catch (...) {
        return fallback;
    }
}

bool extractBoolField(const std::string& responseText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = responseText.find(key);
    if (keyPos == std::string::npos) {
        return false;
    }
    const auto colonPos = responseText.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return false;
    }
    const auto valueStart = responseText.find_first_not_of(" \t\r\n", colonPos + 1);
    return valueStart != std::string::npos && responseText.compare(valueStart, 4, "true") == 0;
}

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
    std::vector<std::thread> threads;

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        for (auto& entry : stopFlags_) {
            *entry.second = true;
        }
        for (auto& entry : workerThreads_) {
            if (entry.second.joinable()) {
                threads.push_back(std::move(entry.second));
            }
        }
        workerThreads_.clear();
        stopFlags_.clear();
        activeDevices_.clear();
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    Logger::info("DataTask stopped");
}

std::vector<DeviceStatus> DataTask::getStatus() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    std::vector<DeviceStatus> statuses;
    statuses.reserve(statuses_.size());
    for (const auto& entry : statuses_) {
        statuses.push_back(entry.second);
    }
    return statuses;
}

// Initialize model-based DataPoint rows for one device.
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

// Start one named device collection worker.
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

        std::thread staleThread;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (activeDevices_.count(deviceName) != 0) {
                throw std::runtime_error("device collection is already running: " + deviceName);
            }

            auto staleThreadIt = workerThreads_.find(deviceName);
            if (staleThreadIt != workerThreads_.end()) {
                staleThread = std::move(staleThreadIt->second);
                workerThreads_.erase(staleThreadIt);
            }
            stopFlags_.erase(deviceName);
        }

        if (staleThread.joinable()) {
            staleThread.join();
        }

        auto stopFlag = std::make_shared<std::atomic_bool>(false);
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            activeDevices_.insert(deviceName);
            stopFlags_[deviceName] = stopFlag;
            workerThreads_[deviceName] = std::thread(&DataTask::runDeviceTask, this, device, stopFlag);
        }

        result.msg = "device collection started: " + deviceName;
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        activeDevices_.erase(deviceName);
        stopFlags_.erase(deviceName);
        result.code = 500;
        result.msg = ex.what();
    }

    return result;
}

// Stop the current named device collection worker.
CommandResult DataTask::stopDeviceCollect(const std::string& deviceName, const std::string& mid) {
    CommandResult result;
    result.mid = mid;

    if (!sqliteManager_.deviceExists(deviceName)) {
        result.code = 500;
        result.msg = "device not found: " + deviceName;
        return result;
    }

    std::thread threadToJoin;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (activeDevices_.count(deviceName) == 0) {
            result.msg = "device collection is not running: " + deviceName;
            return result;
        }

        auto flagIt = stopFlags_.find(deviceName);
        if (flagIt != stopFlags_.end()) {
            *flagIt->second = true;
        }

        auto threadIt = workerThreads_.find(deviceName);
        if (threadIt != workerThreads_.end()) {
            threadToJoin = std::move(threadIt->second);
            workerThreads_.erase(threadIt);
        }
        stopFlags_.erase(deviceName);
    }

    if (threadToJoin.joinable()) {
        threadToJoin.join();
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        activeDevices_.erase(deviceName);
    }
    result.msg = "device collection stopped: " + deviceName;
    Logger::info(result.msg);
    return result;
}

//閲囬泦鏁版嵁

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
            const auto cid = makeCid(device.name);
            const auto query = buildQuery(cid, points);
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
            processResponse(device, points, response);
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
            status.machineConnected = extractBoolField(response, "machineConnected");
            status.failCount = 0;
            status.lastSuccessTime = nowSeconds();
            status.error.clear();
            setLatestStatus(status);
            Logger::info("DataTask collection succeeded for " + device.name);
        } catch (const std::exception& ex) {
            ++failCount;
            status.connected = false;
            status.machineConnected = false;
            status.failCount = failCount;
            status.error = stage + " failed: " + ex.what();
            setLatestStatus(status);
            Logger::error("DataTask collection failed for " + device.name +
                          ", stage=" + stage + ": " + ex.what());
            if (failCount >= 5) {
                Logger::error("DataTask stopped after 5 consecutive failures for " + device.name);
                break;
            }
        }

        sleepUntilNextCycle(device.sampleIntervalMs, stopFlag);
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        activeDevices_.erase(device.name);
        stopFlags_.erase(device.name);
    }
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

void DataTask::setLatestStatus(const DeviceStatus& status) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    statuses_[status.deviceName] = status;
}

void DataTask::processResponse(const Device& device,
                               const std::vector<DataPoint>& points,
                               const std::string& responseText) {
    std::vector<ResourceRecord> records;
    const double fallbackTime = nowSeconds();

    for (const auto& point : points) {
        const auto objectText = extractObjectForPath(responseText, point.namePath);
        if (objectText.empty()) {
            continue;
        }

        ResourceRecord record;
        record.deviceName = device.name;
        record.dataItemNamePath = point.namePath;
        record.data = extractArrayFieldText(objectText, "value");
        record.devTime = extractNumberField(objectText, "dev_time", fallbackTime);
        record.svrTime = extractNumberField(objectText, "svr_time", fallbackTime);

        record.dataItemPath = extractStringField(objectText, "item_path");
        if (record.dataItemPath.empty()) {
            record.dataItemPath = point.itemPath;
        }

        record.dataItemName = extractStringField(objectText, "item_name");
        if (record.dataItemName.empty()) {
            record.dataItemName = point.itemName;
        }

        record.dataItemDescription = extractStringField(objectText, "item_description");
        if (record.dataItemDescription.empty()) {
            record.dataItemDescription = point.itemDescription;
        }

        if (!record.data.empty()) {
            records.push_back(record);
        }
    }

    if (records.empty()) {
        throw std::runtime_error("response did not contain resource records");
    }

    sqliteManager_.insertResources(records);
    Logger::info("DataTask inserted " + std::to_string(records.size()) + " Resources records");
}
