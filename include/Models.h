#pragma once

#include <string>

struct Device {
    int id = 0;
    std::string name;
    std::string ip;
    int port = 0;
    int sampleIntervalMs = 0;
    int status = 1;
    std::string model;
    std::string modelVersion;
    std::string driverPath;
};

struct DataPoint {
    int id = 0;
    int deviceId = 0;
    std::string deviceName;
    std::string namePath;
    std::string itemPath;
    std::string itemName;
    std::string itemDescription;
};

struct ResourceRecord {
    std::string deviceName;
    std::string dataItemNamePath;
    std::string data;
    double devTime = 0.0;
    double svrTime = 0.0;
    std::string dataItemPath;
    std::string dataItemName;
    std::string dataItemDescription;
};

struct DeviceStatus {
    bool connected = false;
    bool machineConnected = false;
    int failCount = 0;
    double lastSuccessTime = 0.0;
    std::string error;
};

struct DeviceUpsert {
    std::string oldName;
    std::string name;
    std::string ip;
    int port = 0;
    int sampleIntervalMs = 0;
    int status = 1;
    std::string model;
    std::string modelVersion;
    std::string driverPath;
};

struct CommandResult {
    int code = 200;
    std::string msg;
    std::string mid;

    bool ok() const {
        return code == 200;
    }
};
