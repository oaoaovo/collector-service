#pragma once

#include <string>

namespace http_support {

std::string commandEnvelope(const std::string& category, const std::string& deviceName, const std::string& mid);

} // namespace http_support
