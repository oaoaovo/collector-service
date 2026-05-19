#include "HttpServer.h"

#include "CommandProcessor.h"
#include "DataTask.h"
#include "Logger.h"
#include "SQLiteManager.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

namespace {

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

std::string jsonSuccess(const std::string& data) {
    return "{\"success\":true,\"data\":" + data + "}";
}

std::string jsonError(const std::string& message) {
    return "{\"success\":false,\"error\":\"" + escapeJson(message) + "\"}";
}

std::string commandEnvelope(const std::string& category, const std::string& deviceName, const std::string& mid) {
    std::ostringstream out;
    out << "{\"type\":\"json\",\"uid\":\"http\",\"pid\":\"http\",\"cmd\":{\"category\":\""
        << escapeJson(category) << "\",\"params\":{";
    if (!deviceName.empty()) {
        out << "\"device_name\":\"" << escapeJson(deviceName) << "\"";
    }
    out << "}},\"mid\":\"" << escapeJson(mid) << "\"}";
    return out.str();
}

std::string toJson(const Device& device) {
    std::ostringstream out;
    out << "{\"id\":" << device.id
        << ",\"name\":\"" << escapeJson(device.name)
        << "\",\"ip\":\"" << escapeJson(device.ip)
        << "\",\"port\":" << device.port
        << ",\"sampleIntervalMs\":" << device.sampleIntervalMs
        << ",\"status\":" << device.status
        << ",\"model\":\"" << escapeJson(device.model)
        << "\",\"model_version\":\"" << escapeJson(device.modelVersion)
        << "\",\"driver_path\":\"" << escapeJson(device.driverPath) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<Device>& devices) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < devices.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(devices[i]);
    }
    out << "]";
    return out.str();
}

std::string toJson(const DeviceStatus& status) {
    std::ostringstream out;
    out << "{\"device_name\":\"" << escapeJson(status.deviceName)
        << "\",\"connected\":" << (status.connected ? "true" : "false")
        << ",\"machineConnected\":" << (status.machineConnected ? "true" : "false")
        << ",\"failCount\":" << status.failCount
        << ",\"lastSuccessTime\":" << status.lastSuccessTime
        << ",\"error\":\"" << escapeJson(status.error) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<DeviceStatus>& statuses) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < statuses.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(statuses[i]);
    }
    out << "]";
    return out.str();
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

std::string toJson(const ResourceRecord& record) {
    std::ostringstream out;
    out << "{\"device_name\":\"" << escapeJson(record.deviceName)
        << "\",\"dataItemNamePath\":\"" << escapeJson(record.dataItemNamePath)
        << "\",\"data\":" << (record.data.empty() ? "[]" : record.data)
        << ",\"devTime\":" << record.devTime
        << ",\"svrTime\":" << record.svrTime
        << ",\"dataItemPath\":\"" << escapeJson(record.dataItemPath)
        << "\",\"dataItemName\":\"" << escapeJson(record.dataItemName)
        << "\",\"dataItemDescription\":\"" << escapeJson(record.dataItemDescription) << "\"}";
    return out.str();
}

std::string toJson(const std::vector<ResourceRecord>& records) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << toJson(records[i]);
    }
    out << "]";
    return out.str();
}

http::response<http::string_body> makeResponse(http::status status,
                                               const std::string& body,
                                               unsigned version,
                                               bool keepAlive) {
    http::response<http::string_body> response{status, version};
    response.set(http::field::server, "collector-service");
    response.set(http::field::content_type, "application/json");
    response.keep_alive(keepAlive);
    response.body() = body;
    response.prepare_payload();
    return response;
}

} // namespace

HttpServer::HttpServer(std::string host,
                       int port,
                       SQLiteManager& sqliteManager,
                       DataTask& dataTask,
                       CommandProcessor& commandProcessor)
    : host_(std::move(host)),
      port_(port),
      sqliteManager_(sqliteManager),
      dataTask_(dataTask),
      commandProcessor_(commandProcessor),
      ioContext_(std::make_unique<asio::io_context>()) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_.exchange(true)) {
        return;
    }

    serverThread_ = std::thread(&HttpServer::run, this);
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (ioContext_ != nullptr) {
        ioContext_->stop();
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void HttpServer::run() {
    try {
        ioContext_ = std::make_unique<asio::io_context>();
        const auto address = asio::ip::make_address(host_);
        tcp::acceptor acceptor(*ioContext_, tcp::endpoint(address, static_cast<unsigned short>(port_)));
        acceptor.non_blocking(true);
        Logger::info("HttpServer listening on http://" + host_ + ":" + std::to_string(port_));

        while (running_) {
            beast::error_code ec;
            tcp::socket socket(*ioContext_);
            acceptor.accept(socket, ec);
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (ec) {
                if (running_) {
                    Logger::error("HttpServer accept failed: " + ec.message());
                }
                continue;
            }

            std::thread(&HttpServer::handleSession, this, std::move(socket)).detach();
        }
    } catch (const std::exception& ex) {
        Logger::error(std::string("HttpServer failed: ") + ex.what());
        running_ = false;
    }
}

void HttpServer::handleSession(tcp::socket socket) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    try {
        while (running_) {
            http::request<http::string_body> request;
            http::read(socket, buffer, request, ec);
            if (ec == http::error::end_of_stream) {
                break;
            }
            if (ec) {
                Logger::warn("HttpServer read failed: " + ec.message());
                break;
            }

            const auto method = request.method();
            const auto target = std::string(request.target());
            const auto path = pathOnly(target);
            std::string body;
            http::status status = http::status::ok;

            try {
                if (method == http::verb::get && path == "/health") {
                    body = "{\"success\":true,\"service\":\"collector-service\"}";
                } else if (method == http::verb::get && path == "/devices") {
                    body = jsonSuccess(toJson(sqliteManager_.getDevices()));
                } else if (method == http::verb::get && (path == "/status" || path == "/devices/status")) {
                    const auto deviceName = queryParam(target, "device");
                    const auto statuses = dataTask_.getStatus();
                    if (deviceName.empty()) {
                        body = jsonSuccess(toJson(statuses));
                    } else {
                        body = jsonSuccess(toJson(findStatusOrDefault(sqliteManager_, statuses, deviceName)));
                    }
                } else if (method == http::verb::get && path == "/latest") {
                    const auto deviceName = queryParam(target, "device");
                    if (deviceName.empty()) {
                        status = http::status::bad_request;
                        body = jsonError("device query parameter is required");
                    } else {
                        body = jsonSuccess(toJson(sqliteManager_.getLatestResources(deviceName)));
                    }
                } else if (method == http::verb::get && path.rfind("/devices/", 0) == 0 &&
                           path.find("/status") != std::string::npos) {
                    const auto deviceName = betweenPath(path, "/devices/", "/status");
                    const auto statuses = dataTask_.getStatus();
                    body = jsonSuccess(toJson(findStatusOrDefault(sqliteManager_, statuses, deviceName)));
                } else if (method == http::verb::get && path.rfind("/devices/", 0) == 0 &&
                           path.find("/resources/latest") != std::string::npos) {
                    const auto deviceName = betweenPath(path, "/devices/", "/resources/latest");
                    body = jsonSuccess(toJson(sqliteManager_.getLatestResources(deviceName)));
                } else if (method == http::verb::post && path == "/commands") {
                    body = commandProcessor_.handle(request.body());
                } else if (method == http::verb::post && path == "/tasks/start") {
                    const auto deviceName = queryParam(target, "device");
                    const auto category = deviceName.empty() ? "start_all" : "start_device_collect";
                    body = commandProcessor_.handle(commandEnvelope(category, deviceName, "http-start"));
                } else if (method == http::verb::post && path == "/tasks/stop") {
                    const auto deviceName = queryParam(target, "device");
                    const auto category = deviceName.empty() ? "stop_all" : "stop_device_collect";
                    body = commandProcessor_.handle(commandEnvelope(category, deviceName, "http-stop"));
                } else {
                    status = http::status::not_found;
                    body = jsonError("route not found: " + path);
                }
            } catch (const std::exception& ex) {
                status = http::status::internal_server_error;
                body = jsonError(ex.what());
            }

            auto response = makeResponse(status, body, request.version(), request.keep_alive());
            http::write(socket, response, ec);
            if (ec) {
                Logger::warn("HttpServer write failed: " + ec.message());
                break;
            }
            if (!request.keep_alive()) {
                break;
            }
        }
    } catch (const std::exception& ex) {
        Logger::warn(std::string("HttpServer session failed: ") + ex.what());
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}
