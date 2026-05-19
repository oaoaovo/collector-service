#include "http/HttpRouter.h"

#include "CommandProcessor.h"
#include "DataTask.h"
#include "SQLiteManager.h"
#include "http/HttpCommandAdapter.h"
#include "http/HttpJsonSerializer.h"
#include "http/HttpResponse.h"

#include <stdexcept>
#include <string>

namespace http_support {
namespace {

std::string urlDecode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const auto hex = value.substr(i + 1, 2);
            try {
                decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
                i += 2;
                continue;
            } catch (...) {
            }
        }
        decoded += value[i] == '+' ? ' ' : value[i];
    }
    return decoded;
}

std::string pathOnly(const std::string& target) {
    const auto queryPos = target.find('?');
    return queryPos == std::string::npos ? target : target.substr(0, queryPos);
}

std::string queryParam(const std::string& target, const std::string& name) {
    const auto queryPos = target.find('?');
    if (queryPos == std::string::npos) {
        return {};
    }

    const std::string key = name + "=";
    std::size_t pos = queryPos + 1;
    while (pos < target.size()) {
        const auto next = target.find('&', pos);
        const auto part = target.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (part.rfind(key, 0) == 0) {
            return urlDecode(part.substr(key.size()));
        }
        if (next == std::string::npos) {
            break;
        }
        pos = next + 1;
    }
    return {};
}

std::string betweenPath(const std::string& path, const std::string& prefix, const std::string& suffix) {
    if (path.rfind(prefix, 0) != 0 || path.size() < prefix.size() + suffix.size()) {
        return {};
    }
    if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return {};
    }
    return urlDecode(path.substr(prefix.size(), path.size() - prefix.size() - suffix.size()));
}

DeviceStatus findStatusOrDefault(SQLiteManager& sqliteManager,
                                 const std::vector<DeviceStatus>& statuses,
                                 const std::string& deviceName) {
    for (const auto& status : statuses) {
        if (status.deviceName == deviceName) {
            return status;
        }
    }
    if (!sqliteManager.deviceExists(deviceName)) {
        throw std::runtime_error("device not found: " + deviceName);
    }

    DeviceStatus status;
    status.deviceName = deviceName;
    return status;
}

} // namespace

HttpRouter::HttpRouter(SQLiteManager& sqliteManager, DataTask& dataTask, CommandProcessor& commandProcessor)
    : sqliteManager_(sqliteManager), dataTask_(dataTask), commandProcessor_(commandProcessor) {}

beast_http::response<beast_http::string_body> HttpRouter::route(
    const beast_http::request<beast_http::string_body>& request) {
    const auto method = request.method();
    const auto target = std::string(request.target());
    const auto path = pathOnly(target);
    std::string body;
    beast_http::status status = beast_http::status::ok;

    try {
        if (method == beast_http::verb::get && path == "/health") {
            body = "{\"success\":true,\"service\":\"collector-service\"}";
        } else if (method == beast_http::verb::get && path == "/devices") {
            body = jsonSuccess(toJson(sqliteManager_.getDevices()));
        } else if (method == beast_http::verb::get && (path == "/status" || path == "/devices/status")) {
            const auto deviceName = queryParam(target, "device");
            const auto statuses = dataTask_.getStatus();
            if (deviceName.empty()) {
                body = jsonSuccess(toJson(statuses));
            } else {
                body = jsonSuccess(toJson(findStatusOrDefault(sqliteManager_, statuses, deviceName)));
            }
        } else if (method == beast_http::verb::get && path == "/latest") {
            const auto deviceName = queryParam(target, "device");
            if (deviceName.empty()) {
                status = beast_http::status::bad_request;
                body = jsonError("device query parameter is required");
            } else {
                body = jsonSuccess(toJson(sqliteManager_.getLatestResources(deviceName)));
            }
        } else if (method == beast_http::verb::get && path.rfind("/devices/", 0) == 0 &&
                   path.find("/status") != std::string::npos) {
            const auto deviceName = betweenPath(path, "/devices/", "/status");
            const auto statuses = dataTask_.getStatus();
            body = jsonSuccess(toJson(findStatusOrDefault(sqliteManager_, statuses, deviceName)));
        } else if (method == beast_http::verb::get && path.rfind("/devices/", 0) == 0 &&
                   path.find("/resources/latest") != std::string::npos) {
            const auto deviceName = betweenPath(path, "/devices/", "/resources/latest");
            body = jsonSuccess(toJson(sqliteManager_.getLatestResources(deviceName)));
        } else if (method == beast_http::verb::post && path == "/commands") {
            body = commandProcessor_.handle(request.body());
        } else if (method == beast_http::verb::post && path == "/tasks/start") {
            const auto deviceName = queryParam(target, "device");
            const auto category = deviceName.empty() ? "start_all" : "start_device_collect";
            body = commandProcessor_.handle(commandEnvelope(category, deviceName, "http-start"));
        } else if (method == beast_http::verb::post && path == "/tasks/stop") {
            const auto deviceName = queryParam(target, "device");
            const auto category = deviceName.empty() ? "stop_all" : "stop_device_collect";
            body = commandProcessor_.handle(commandEnvelope(category, deviceName, "http-stop"));
        } else {
            status = beast_http::status::not_found;
            body = jsonError("route not found: " + path);
        }
    } catch (const std::exception& ex) {
        status = beast_http::status::internal_server_error;
        body = jsonError(ex.what());
    }

    return makeResponse(status, body, request.version(), request.keep_alive());
}

} // namespace http_support
