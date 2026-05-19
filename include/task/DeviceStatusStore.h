#pragma once

#include "Models.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace task {

class DeviceStatusStore {
public:
    void requestStopAll();
    std::vector<std::thread> takeAllThreads();
    void clearAllWorkers();
    std::vector<DeviceStatus> getStatus() const;
    bool isActive(const std::string& deviceName) const;
    std::thread takeThread(const std::string& deviceName);
    void eraseStopFlag(const std::string& deviceName);
    void addWorker(const std::string& deviceName, std::shared_ptr<std::atomic_bool> stopFlag, std::thread thread);
    void markStartFailed(const std::string& deviceName);
    bool requestStop(const std::string& deviceName);
    void markStopped(const std::string& deviceName);
    void setLatestStatus(const DeviceStatus& status);

private:
    mutable std::mutex mutex_;
    std::map<std::string, DeviceStatus> statuses_;
    std::set<std::string> activeDevices_;
    std::map<std::string, std::thread> workerThreads_;
    std::map<std::string, std::shared_ptr<std::atomic_bool>> stopFlags_;
};

} // namespace task
