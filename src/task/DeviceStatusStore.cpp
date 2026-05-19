#include "task/DeviceStatusStore.h"

namespace task {

void DeviceStatusStore::requestStopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& entry : stopFlags_) {
        *entry.second = true;
    }
}

std::vector<std::thread> DeviceStatusStore::takeAllThreads() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::thread> threads;
    for (auto& entry : workerThreads_) {
        if (entry.second.joinable()) {
            threads.push_back(std::move(entry.second));
        }
    }
    workerThreads_.clear();
    return threads;
}

void DeviceStatusStore::clearAllWorkers() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopFlags_.clear();
    activeDevices_.clear();
}

std::vector<DeviceStatus> DeviceStatusStore::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DeviceStatus> statuses;
    statuses.reserve(statuses_.size());
    for (const auto& entry : statuses_) {
        statuses.push_back(entry.second);
    }
    return statuses;
}

bool DeviceStatusStore::isActive(const std::string& deviceName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeDevices_.count(deviceName) != 0;
}

std::thread DeviceStatusStore::takeThread(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::thread thread;
    auto threadIt = workerThreads_.find(deviceName);
    if (threadIt != workerThreads_.end()) {
        thread = std::move(threadIt->second);
        workerThreads_.erase(threadIt);
    }
    return thread;
}

void DeviceStatusStore::eraseStopFlag(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    stopFlags_.erase(deviceName);
}

void DeviceStatusStore::addWorker(const std::string& deviceName,
                                  std::shared_ptr<std::atomic_bool> stopFlag,
                                  std::thread thread) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeDevices_.insert(deviceName);
    stopFlags_[deviceName] = std::move(stopFlag);
    workerThreads_[deviceName] = std::move(thread);
}

void DeviceStatusStore::markStartFailed(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeDevices_.erase(deviceName);
    stopFlags_.erase(deviceName);
}

bool DeviceStatusStore::requestStop(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (activeDevices_.count(deviceName) == 0) {
        return false;
    }

    auto flagIt = stopFlags_.find(deviceName);
    if (flagIt != stopFlags_.end()) {
        *flagIt->second = true;
    }
    return true;
}

void DeviceStatusStore::markStopped(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeDevices_.erase(deviceName);
    stopFlags_.erase(deviceName);
}

void DeviceStatusStore::setLatestStatus(const DeviceStatus& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    statuses_[status.deviceName] = status;
}

} // namespace task
