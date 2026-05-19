#include "command/CommandParser.h"

#include <stdexcept>

namespace command {

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

int extractIntField(const std::string& text, const std::string& fieldName, int fallback) {
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

} // namespace command
