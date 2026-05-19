#include "http/HttpJsonSerializer.h"

#include "util/JsonUtil.h"

#include <sstream>

namespace http_support {

std::string toJson(const Device& device) {
    std::ostringstream out;
    out << "{\"id\":" << device.id
        << ",\"name\":\"" << util::escapeJson(device.name)
        << "\",\"ip\":\"" << util::escapeJson(device.ip)
        << "\",\"port\":" << device.port
        << ",\"sampleIntervalMs\":" << device.sampleIntervalMs
        << ",\"status\":" << device.status
        << ",\"model\":\"" << util::escapeJson(device.model)
        << "\",\"model_version\":\"" << util::escapeJson(device.modelVersion)
        << "\",\"driver_path\":\"" << util::escapeJson(device.driverPath) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<Device>& devices) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < devices.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(devices[i]);
    }
    out << "]";
    return out.str();
}

std::string toJson(const DeviceStatus& status) {
    std::ostringstream out;
    out << "{\"device_name\":\"" << util::escapeJson(status.deviceName)
        << "\",\"connected\":" << util::boolText(status.connected)
        << ",\"machineConnected\":" << util::boolText(status.machineConnected)
        << ",\"failCount\":" << status.failCount
        << ",\"lastSuccessTime\":" << status.lastSuccessTime
        << ",\"error\":\"" << util::escapeJson(status.error) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<DeviceStatus>& statuses) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < statuses.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(statuses[i]);
    }
    out << "]";
    return out.str();
}

std::string toJson(const ResourceRecord& record) {
    std::ostringstream out;
    out << "{\"device_name\":\"" << util::escapeJson(record.deviceName)
        << "\",\"dataItemNamePath\":\"" << util::escapeJson(record.dataItemNamePath)
        << "\",\"data\":" << (record.data.empty() ? "[]" : record.data)
        << ",\"devTime\":" << record.devTime
        << ",\"svrTime\":" << record.svrTime
        << ",\"dataItemPath\":\"" << util::escapeJson(record.dataItemPath)
        << "\",\"dataItemName\":\"" << util::escapeJson(record.dataItemName)
        << "\",\"dataItemDescription\":\"" << util::escapeJson(record.dataItemDescription) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<ResourceRecord>& records) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(records[i]);
    }
    out << "]";
    return out.str();
}

} // namespace http_support
