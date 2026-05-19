#include "command/CommandJsonSerializer.h"

#include "util/JsonUtil.h"

#include <sstream>

namespace command {

std::string toJson(const CommandResult& result) {
    std::ostringstream out;
    out << "{\"code\":" << result.code
        << ",\"msg\":\"" << util::escapeJson(result.msg)
        << "\",\"mid\":\"" << util::escapeJson(result.mid) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<DeviceStatus>& statuses, const std::string& mid) {
    std::ostringstream out;
    out << "{\"code\":200,\"msg\":\"ok\",\"mid\":\"" << util::escapeJson(mid) << "\",\"data\":[";
    for (std::size_t i = 0; i < statuses.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& status = statuses[i];
        out << "{\"device_name\":\"" << util::escapeJson(status.deviceName)
            << "\",\"connected\":" << util::boolText(status.connected)
            << ",\"machineConnected\":" << util::boolText(status.machineConnected)
            << ",\"failCount\":" << status.failCount
            << ",\"lastSuccessTime\":" << status.lastSuccessTime
            << ",\"error\":\"" << util::escapeJson(status.error) << "\"}";
    }
    out << "]}";
    return out.str();
}

} // namespace command
