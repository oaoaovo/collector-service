#include "DriverClient.h"

#include "Logger.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <stdexcept>
#include <utility>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

struct DriverClient::Impl {
    asio::io_context ioContext;
    websocket::stream<tcp::socket> ws;
    bool connected = false;

    Impl() : ws(ioContext) {}
};

DriverClient::DriverClient(std::string host, int port)
    : host_(std::move(host)), port_(port), impl_(std::make_unique<Impl>()) {}

DriverClient::~DriverClient() {
    close();
}

bool DriverClient::connect() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    if (impl_->connected) {
        return true;
    }

    try {
        tcp::resolver resolver(impl_->ioContext);
        const auto results = resolver.resolve(host_, std::to_string(port_));
        asio::connect(impl_->ws.next_layer(), results);
        impl_->ws.handshake(host_ + ":" + std::to_string(port_), "/");
        impl_->connected = true;
        Logger::info("DriverClient connected to ws://" + host_ + ":" + std::to_string(port_));
        return true;
    } catch (const std::exception& ex) {
        Logger::error(std::string("DriverClient connect failed: ") + ex.what());
        impl_->connected = false;
        impl_ = std::make_unique<Impl>();
        return false;
    }
}

std::string DriverClient::sendQuery(const std::string& queryText) {
    if (!impl_->connected) {
        throw std::runtime_error("DriverClient is not connected");
    }

    beast::flat_buffer buffer;
    try {
        impl_->ws.text(true);
        impl_->ws.write(asio::buffer(queryText));
        impl_->ws.read(buffer);
        return beast::buffers_to_string(buffer.data());
    } catch (const std::exception& ex) {
        close();
        throw std::runtime_error(std::string("DriverClient sendQuery failed: ") + ex.what());
    }
}

void DriverClient::close() {
    if (!impl_) {
        return;
    }

    if (impl_->connected) {
        beast::error_code ec;
        impl_->ws.close(websocket::close_code::normal, ec);
        if (ec) {
            Logger::warn("DriverClient close warning: " + ec.message());
        }
    }
    impl_ = std::make_unique<Impl>();
}
