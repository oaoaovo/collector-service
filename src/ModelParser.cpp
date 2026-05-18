#include "ModelParser.h"

#include <sstream>
#include <stdexcept>

namespace {

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

int indentOf(const std::string& line) {
    int indent = 0;
    for (const char ch : line) {
        if (ch != ' ') {
            break;
        }
        ++indent;
    }
    return indent;
}

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string valueAfterColon(const std::string& line) {
    const auto pos = line.find(':');
    if (pos == std::string::npos) {
        return {};
    }
    auto value = trim(line.substr(pos + 1));
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

} // namespace

std::vector<DataPoint> ModelParser::parseModelToDataPoints(const Device& device, const std::string& modelText) const {
    auto points = parseDataItems(device, modelText);
    if (points.empty()) {
        throw std::runtime_error("model has no DataItem: " + device.model);
    }
    return points;
}

std::vector<DataPoint> ModelParser::parseDataItems(const Device& device, const std::string& modelText) const {
    std::istringstream input(modelText);
    std::vector<DataPoint> points;
    std::string currentComponent;
    DataPoint currentPoint;
    bool inDataItem = false;
    bool hasCurrentPoint = false;

    auto flushPoint = [&]() {
        if (!hasCurrentPoint || currentPoint.itemName.empty()) {
            return;
        }
        currentPoint.deviceId = device.id;
        currentPoint.deviceName = device.name;
        if (currentPoint.namePath.empty()) {
            currentPoint.namePath = "/" + currentComponent + "/" + currentPoint.itemName;
        }
        if (currentPoint.itemPath.empty()) {
            currentPoint.itemPath = "/DV:" + device.name + "/CP:" + currentComponent + "/DT:" + currentPoint.itemName;
        }
        points.push_back(currentPoint);
        currentPoint = DataPoint{};
        hasCurrentPoint = false;
    };

    std::string line;
    while (std::getline(input, line)) {
        const auto stripped = trim(line);
        if (stripped.empty()) {
            continue;
        }

        const int indent = indentOf(line);
        if (stripped == "DataItem:") {
            inDataItem = true;
            continue;
        }

        if (startsWith(stripped, "- Name:")) {
            const auto name = valueAfterColon(stripped);
            if (inDataItem && indent >= 8) {
                flushPoint();
                currentPoint = DataPoint{};
                currentPoint.itemName = name;
                currentPoint.namePath = "/" + currentComponent + "/" + name;
                currentPoint.itemPath = "/DV:" + device.name + "/CP:" + currentComponent + "/DT:" + name;
                hasCurrentPoint = true;
            } else {
                flushPoint();
                currentComponent = name;
                inDataItem = false;
            }
            continue;
        }

        if (!hasCurrentPoint) {
            continue;
        }

        if (startsWith(stripped, "Description:")) {
            currentPoint.itemDescription = valueAfterColon(stripped);
        } else if (startsWith(stripped, "NamePath:") || startsWith(stripped, "namePath:") ||
                   startsWith(stripped, "Path:") || startsWith(stripped, "path:")) {
            currentPoint.namePath = valueAfterColon(stripped);
        }
    }

    flushPoint();
    return points;
}
