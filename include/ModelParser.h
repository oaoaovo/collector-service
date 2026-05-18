#pragma once

#include "Models.h"

#include <string>
#include <vector>

class ModelParser {
public:
    std::vector<DataPoint> parseModelToDataPoints(const Device& device, const std::string& modelText) const;

private:
    std::vector<DataPoint> parseDataItems(const Device& device, const std::string& modelText) const;
};
