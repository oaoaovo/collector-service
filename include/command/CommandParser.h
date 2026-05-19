#pragma once

#include "Models.h"

#include <string>

namespace command {

std::string extractStringField(const std::string& text, const std::string& fieldName);
bool hasField(const std::string& text, const std::string& fieldName);
int extractIntField(const std::string& text, const std::string& fieldName, int fallback = 0);
void parseAddr(const std::string& addr, std::string& ip, int& port);
DeviceUpsert parseDeviceUpsert(const std::string& text);

} // namespace command
