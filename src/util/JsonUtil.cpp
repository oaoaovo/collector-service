#include "util/JsonUtil.h"

namespace util {

std::string escapeJson(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

std::string quoteJson(const std::string& value) {
    return "\"" + escapeJson(value) + "\"";
}

std::string boolText(bool value) {
    return value ? "true" : "false";
}

std::string arrayOf(const std::vector<std::string>& items) {
    std::string result = "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += items[i];
    }
    result += "]";
    return result;
}

} // namespace util
