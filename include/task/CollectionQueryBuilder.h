#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace task {

std::string makeCid(const std::string& deviceName);
std::string buildQuery(const std::string& cid, const std::vector<DataPoint>& points);

} // namespace task
