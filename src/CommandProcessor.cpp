#include "CommandProcessor.h"

#include "DataTask.h"
#include "Logger.h"
#include "SQLiteManager.h"

#include <sstream>
#include <stdexcept>

namespace {

std::string escapeJson(const std::string& value) {
    std::string escaped;
    for (const char ch : value) {
        if (ch == '\\') {
            escaped += "\\\\";
        } else if (ch == '"') {
            escaped += "\\\"";
        } else if (ch == '\n') {
            escaped += "\\n";
        } else {
            escaped += ch;
        }
    }
    return escaped;
}

std::string extractStringField(const std::string& text, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = text.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }
    const auto colonPos = text.find(':', keyPos + key.size());
    const auto quotePos = text.find('"', colonPos);
    if (colonPos == std::string::npos || quotePos == std::string::npos) {
        return {};
    }

    std::string value;
    bool escaped = false;
    for (auto pos = quotePos + 1; pos < text.size(); ++pos) {
        const char ch = text[pos];
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

bool hasField(const std::string& text, const std::string& fieldName) {
    return text.find("\"" + fieldName + "\"") != std::string::npos;
}

int extractIntField(const std::string& text, const std::string& fieldName, int fallback = 0) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = text.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }
    const auto colonPos = text.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return fallback;
    }
    const auto valueStart = text.find_first_of("-0123456789", colonPos + 1);
    if (valueStart == std::string::npos) {
        return fallback;
    }
    const auto valueEnd = text.find_first_not_of("-0123456789", valueStart);
    return std::stoi(text.substr(valueStart, valueEnd - valueStart));
}

void parseAddr(const std::string& addr, std::string& ip, int& port) {
    if (addr.empty()) {
        return;
    }
    const auto pos = addr.find_last_of(':');
    if (pos == std::string::npos) {
        ip = addr;
        return;
    }
    ip = addr.substr(0, pos);
    port = std::stoi(addr.substr(pos + 1));
}

DeviceUpsert parseDeviceUpsert(const std::string& text) {
    DeviceUpsert device;
    device.oldName = extractStringField(text, "old_device_name");
    device.name = extractStringField(text, "device_name");
    device.model = extractStringField(text, "model");
    device.modelVersion = extractStringField(text, "model_version");
    device.driverPath = extractStringField(text, "driver_path");
    device.sampleIntervalMs = extractIntField(text, "SampleInterval", 1000);
    device.status = extractIntField(text, "status", 1);
    device.status = extractIntField(text, "Status", device.status);
    device.ip = extractStringField(text, "IpAddr");
    if (device.ip.empty()) {
        device.ip = extractStringField(text, "ip");
    }
    device.port = extractIntField(text, "Port", 0);

    auto addr = extractStringField(text, "addr");
    if (addr.empty()) {
        addr = extractStringField(text, "IpAddr");
    }
    parseAddr(addr, device.ip, device.port);

    if (device.port == 0) {
        device.port = 9001;
    }
    if (device.ip.empty()) {
        device.ip = "127.0.0.1";
    }
    return device;
}

std::string toJson(const CommandResult& result) {
    std::ostringstream out;
    out << "{\"code\":" << result.code
        << ",\"msg\":\"" << escapeJson(result.msg)
        << "\",\"mid\":\"" << escapeJson(result.mid) << "\"}";
    return out.str();
}

CommandResult errorResult(const std::string& mid, const std::string& msg) {
    CommandResult result;
    result.code = 500;
    result.msg = msg;
    result.mid = mid;
    return result;
}

} // namespace

CommandProcessor::CommandProcessor(SQLiteManager& sqliteManager, DataTask& dataTask)
    : sqliteManager_(sqliteManager), dataTask_(dataTask) {}

std::string CommandProcessor::handle(const std::string& requestText) {
    const auto mid = extractStringField(requestText, "mid");
    const auto category = extractStringField(requestText, "category");

    try {
        if (category == "create_device") {
            return toJson(handleCreateDevice(requestText, mid));
        }
        if (category == "delete_device") {
            return toJson(handleDeleteDevice(requestText, mid));
        }
        if (category == "update_device") {
            return toJson(handleUpdateDevice(requestText, mid));
        }
        if (category == "start_device_collect") {
            return toJson(handleStartDeviceCollect(requestText, mid));
        }
        if (category == "stop_device_collect") {
            return toJson(handleStopDeviceCollect(requestText, mid));
        }
        return toJson(errorResult(mid, "unknown cmd category: " + category));
    } catch (const std::exception& ex) {
        return toJson(errorResult(mid, ex.what()));
    }
}

CommandResult CommandProcessor::handleCreateDevice(const std::string& requestText, const std::string& mid) {
    auto device = parseDeviceUpsert(requestText);
    if (device.name.empty()) {
        return errorResult(mid, "device_name is empty");
    }
    if (device.model.empty()) {
        return errorResult(mid, "model is empty");
    }
    if (sqliteManager_.deviceExists(device.name)) {
        return errorResult(mid, "device already exists: " + device.name);
    }

    sqliteManager_.createDevice(device);
    const auto initResult = dataTask_.initDataPointsByDevice(device.name, mid);
    if (!initResult.ok()) {
        Logger::warn("create_device created device but failed to initialize DataPoint: " + initResult.msg);
        return initResult;
    }

    CommandResult result;
    result.mid = mid;
    result.msg = "device created: " + device.name;
    return result;
}

CommandResult CommandProcessor::handleDeleteDevice(const std::string& requestText, const std::string& mid) {
    const auto deviceName = extractStringField(requestText, "device_name");
    if (!sqliteManager_.deviceExists(deviceName)) {
        return errorResult(mid, "device not found: " + deviceName);
    }

    sqliteManager_.deleteDevice(deviceName);
    CommandResult result;
    result.mid = mid;
    result.msg = "device deleted: " + deviceName;
    return result;
}

CommandResult CommandProcessor::handleUpdateDevice(const std::string& requestText, const std::string& mid) {
    auto device = parseDeviceUpsert(requestText);
    if (device.name.empty()) {
        return errorResult(mid, "device_name is empty");
    }
    if (device.model.empty()) {
        return errorResult(mid, "model is empty");
    }
    if (!sqliteManager_.deviceExists(device.oldName.empty() ? device.name : device.oldName)) {
        return errorResult(mid, "device not found: " + device.name);
    }

    const auto before = sqliteManager_.getDeviceByName(device.oldName.empty() ? device.name : device.oldName);
    if (!hasField(requestText, "status") && !hasField(requestText, "Status")) {
        device.status = before.status;
    }
    sqliteManager_.updateDevice(device);
    if (before.model != device.model || before.modelVersion != device.modelVersion) {
        const auto initResult = dataTask_.initDataPointsByDevice(device.name, mid);
        if (!initResult.ok()) {
            return initResult;
        }
    }

    CommandResult result;
    result.mid = mid;
    result.msg = "device updated: " + device.name;
    return result;
}

CommandResult CommandProcessor::handleStartDeviceCollect(const std::string& requestText, const std::string& mid) {
    return dataTask_.startDeviceCollect(extractStringField(requestText, "device_name"), mid);
}

CommandResult CommandProcessor::handleStopDeviceCollect(const std::string& requestText, const std::string& mid) {
    return dataTask_.stopDeviceCollect(extractStringField(requestText, "device_name"), mid);
}
