#include "HttpServer.h"

#include "http/HttpRouter.h"
#include "util/Logger.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

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
    http_support::HttpRouter router(sqliteManager_, dataTask_, commandProcessor_);

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

            auto response = router.route(request);
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
