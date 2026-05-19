#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace http_support {

std::string toJson(const Device& device);
std::string toJson(const std::vector<Device>& devices);
std::string toJson(const DeviceStatus& status);
std::string toJson(const std::vector<DeviceStatus>& statuses);
std::string toJson(const ResourceRecord& record);
std::string toJson(const std::vector<ResourceRecord>& records);

} // namespace http_support
