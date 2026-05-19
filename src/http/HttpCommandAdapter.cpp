#include "http/HttpCommandAdapter.h"

#include "util/JsonUtil.h"

#include <sstream>

namespace http_support {

std::string commandEnvelope(const std::string& category, const std::string& deviceName, const std::string& mid) {
    std::ostringstream out;
    out << "{\"type\":\"json\",\"uid\":\"http\",\"pid\":\"http\",\"cmd\":{\"category\":\""
        << util::escapeJson(category) << "\",\"params\":{";
    if (!deviceName.empty()) {
        out << "\"device_name\":\"" << util::escapeJson(deviceName) << "\"";
    }
    out << "}},\"mid\":\"" << util::escapeJson(mid) << "\"}";
    return out.str();
}

} // namespace http_support
