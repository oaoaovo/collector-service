#include "task/CollectionResponseParser.h"

#include "util/TimeUtil.h"

#include <stdexcept>

namespace task {
namespace {

std::string extractObjectForPath(const std::string& responseText, const std::string& path) {
    const std::string key = "\"" + path + "\"";
    const auto keyPos = responseText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto objectStart = responseText.find('{', keyPos + key.size());
    if (objectStart == std::string::npos) {
        return {};
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (auto pos = objectStart; pos < responseText.size(); ++pos) {
        const char ch = responseText[pos];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = inString;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (inString) {
            continue;
        }
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return responseText.substr(objectStart, pos - objectStart + 1);
            }
        }
    }

    return {};
}

std::string extractStringField(const std::string& objectText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto colonPos = objectText.find(':', keyPos + key.size());
    const auto quotePos = objectText.find('"', colonPos);
    if (colonPos == std::string::npos || quotePos == std::string::npos) {
        return {};
    }

    std::string value;
    bool escaped = false;
    for (auto pos = quotePos + 1; pos < objectText.size(); ++pos) {
        const char ch = objectText[pos];
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

std::string extractArrayFieldText(const std::string& objectText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto arrayStart = objectText.find('[', keyPos + key.size());
    const auto arrayEnd = objectText.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        return {};
    }
    return objectText.substr(arrayStart, arrayEnd - arrayStart + 1);
}

double extractNumberField(const std::string& objectText, const std::string& fieldName, double fallback) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    const auto colonPos = objectText.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return fallback;
    }

    const auto valueStart = objectText.find_first_of("-0123456789", colonPos + 1);
    if (valueStart == std::string::npos) {
        return fallback;
    }

    const auto valueEnd = objectText.find_first_not_of("-0123456789.eE+", valueStart);
    try {
        return std::stod(objectText.substr(valueStart, valueEnd - valueStart));
    } catch (...) {
        return fallback;
    }
}

} // namespace

bool extractBoolField(const std::string& responseText, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = responseText.find(key);
    if (keyPos == std::string::npos) {
        return false;
    }
    const auto colonPos = responseText.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return false;
    }
    const auto valueStart = responseText.find_first_not_of(" \t\r\n", colonPos + 1);
    return valueStart != std::string::npos && responseText.compare(valueStart, 4, "true") == 0;
}

std::vector<ResourceRecord> processResponse(const Device& device,
                                            const std::vector<DataPoint>& points,
                                            const std::string& responseText) {
    std::vector<ResourceRecord> records;
    const double fallbackTime = util::nowSeconds();

    for (const auto& point : points) {
        const auto objectText = extractObjectForPath(responseText, point.namePath);
        if (objectText.empty()) {
            continue;
        }

        ResourceRecord record;
        record.deviceName = device.name;
        record.dataItemNamePath = point.namePath;
        record.data = extractArrayFieldText(objectText, "value");
        record.devTime = extractNumberField(objectText, "dev_time", fallbackTime);
        record.svrTime = extractNumberField(objectText, "svr_time", fallbackTime);

        record.dataItemPath = extractStringField(objectText, "item_path");
        if (record.dataItemPath.empty()) {
            record.dataItemPath = point.itemPath;
        }

        record.dataItemName = extractStringField(objectText, "item_name");
        if (record.dataItemName.empty()) {
            record.dataItemName = point.itemName;
        }

        record.dataItemDescription = extractStringField(objectText, "item_description");
        if (record.dataItemDescription.empty()) {
            record.dataItemDescription = point.itemDescription;
        }

        if (!record.data.empty()) {
            records.push_back(record);
        }
    }

    if (records.empty()) {
        throw std::runtime_error("response did not contain resource records");
    }

    return records;
}

} // namespace task
