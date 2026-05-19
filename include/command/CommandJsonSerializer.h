#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace command {

std::string toJson(const CommandResult& result);
std::string toJson(const std::vector<DeviceStatus>& statuses, const std::string& mid);

} // namespace command
