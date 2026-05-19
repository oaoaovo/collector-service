#include "MockDriverServer.h"

#include "util/JsonUtil.h"
#include "util/Logger.h"
#include "util/TimeUtil.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

namespace {

std::string trim(const std::string& value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }
    return {begin, end};
}

std::string extractStringField(const std::string& json, const std::string& fieldName) {
    const std::string key = "\"" + fieldName + "\"";
    const auto keyPos = json.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto colonPos = json.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return {};
    }

    const auto quotePos = json.find('"', colonPos + 1);
    if (quotePos == std::string::npos) {
        return {};
    }

    std::string value;
    bool escaped = false;
    for (auto pos = quotePos + 1; pos < json.size(); ++pos) {
        const char ch = json[pos];
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
            break;
        }
        value += ch;
    }
    return value;
}

std::vector<std::string> extractQueryPaths(const std::string& json) {
    const std::string key = "\"query\"";
    const auto keyPos = json.find(key);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto arrayStart = json.find('[', keyPos + key.size());
    const auto arrayEnd = json.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        return {};
    }

    std::vector<std::string> paths;
    std::string current;
    bool inString = false;
    bool escaped = false;

    for (auto pos = arrayStart + 1; pos < arrayEnd; ++pos) {
        const char ch = json[pos];
        if (!inString) {
            if (ch == '"') {
                inString = true;
                current.clear();
            }
            continue;
        }

        if (escaped) {
            current += ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            paths.push_back(trim(current));
            inString = false;
            continue;
        }
        current += ch;
    }

    return paths;
}

std::string itemNameFromPath(const std::string& path) {
    const auto slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos || slashPos + 1 >= path.size()) {
        return path;
    }
    return path.substr(slashPos + 1);
}

double valueForPath(const std::string& path) {
    if (path.find("pressure") != std::string::npos) {
        return 1.2;
    }
    if (path.find("temperature") != std::string::npos) {
        return 2.0;
    }
    return 0.0;
}

std::string descriptionForPath(const std::string& path) {
    if (path.find("pressure") != std::string::npos) {
        return "压力";
    }
    if (path.find("temperature") != std::string::npos) {
        return "温度";
    }
    return itemNameFromPath(path);
}

} // namespace

MockDriverServer::MockDriverServer(std::string host, int port)
    : host_(std::move(host)), port_(port), ioContext_(std::make_unique<asio::io_context>()) {}

MockDriverServer::~MockDriverServer() {
    stop();
}

void MockDriverServer::start() {
    if (running_.exchange(true)) {
        return;
    }

    serverThread_ = std::thread(&MockDriverServer::run, this);
}

void MockDriverServer::stop() {
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

void MockDriverServer::run() {
    try {
        ioContext_ = std::make_unique<asio::io_context>();
        const auto address = asio::ip::make_address(host_);
        tcp::acceptor acceptor(*ioContext_, tcp::endpoint(address, static_cast<unsigned short>(port_)));
        acceptor.non_blocking(true);
        Logger::info("MockDriverServer listening on ws://" + host_ + ":" + std::to_string(port_));

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
                    Logger::error("MockDriverServer accept failed: " + ec.message());
                }
                continue;
            }

            std::thread(&MockDriverServer::handleSession, this, std::move(socket)).detach();
        }
    } catch (const std::exception& ex) {
        Logger::error(std::string("MockDriverServer failed: ") + ex.what());
        running_ = false;
    }
}

void MockDriverServer::handleSession(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        while (running_) {
            beast::flat_buffer buffer;
            beast::error_code ec;
            ws.read(buffer, ec);
            if (ec == websocket::error::closed) {
                break;
            }
            if (ec) {
                Logger::warn("MockDriverServer read failed: " + ec.message());
                break;
            }

            const auto message = beast::buffers_to_string(buffer.data());
            const auto response = handleQuery(message);
            ws.text(true);
            ws.write(asio::buffer(response), ec);
            if (ec) {
                Logger::warn("MockDriverServer write failed: " + ec.message());
                break;
            }
        }
    } catch (const std::exception& ex) {
        Logger::warn(std::string("MockDriverServer session failed: ") + ex.what());
    }
}

std::string MockDriverServer::handleQuery(const std::string& message) const {
    std::string cid = extractStringField(message, "cid");
    if (cid.empty()) {
        cid = "mock-cid";
    }

    const auto paths = extractQueryPaths(message);
    const double timestamp = util::nowSeconds();

    std::string response = "{\"cid\":\"" + util::escapeJson(cid) + "\",\"machineConnected\":true,\"data\":{";
    bool first = true;
    for (const auto& path : paths) {
        if (!first) {
            response += ",";
        }
        first = false;

        const auto name = itemNameFromPath(path);
        response += "\"" + util::escapeJson(path) + "\":{";
        response += "\"value\":[" + std::to_string(valueForPath(path)) + "],";
        response += "\"dev_time\":" + std::to_string(timestamp) + ",";
        response += "\"svr_time\":" + std::to_string(timestamp) + ",";
        response += "\"item_path\":\"/DV:mock/CP:mock/DT:" + util::escapeJson(name) + "\",";
        response += "\"item_description\":\"" + util::escapeJson(descriptionForPath(path)) + "\",";
        response += "\"item_name\":\"" + util::escapeJson(name) + "\"";
        response += "}";
    }
    response += "}}";
    return response;
}
