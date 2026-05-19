#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace task {

bool extractBoolField(const std::string& responseText, const std::string& fieldName);
std::vector<ResourceRecord> processResponse(const Device& device,
                                            const std::vector<DataPoint>& points,
                                            const std::string& responseText);

} // namespace task
