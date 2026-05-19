#pragma once

#include <string>
#include <vector>

namespace util {

std::string escapeJson(const std::string& value);
std::string quoteJson(const std::string& value);
std::string boolText(bool value);
std::string arrayOf(const std::vector<std::string>& items);

} // namespace util
