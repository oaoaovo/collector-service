#pragma once

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <thread>

namespace boost {
namespace asio {
class io_context;
} // namespace asio
} // namespace boost

class MockDriverServer {
public:
    MockDriverServer(std::string host, int port);
    ~MockDriverServer();

    MockDriverServer(const MockDriverServer&) = delete;
    MockDriverServer& operator=(const MockDriverServer&) = delete;

    void start();
    void stop();

private:
    void run();
    void handleSession(boost::asio::ip::tcp::socket socket);
    std::string handleQuery(const std::string& message) const;

    std::string host_;
    int port_ = 0;
    std::atomic_bool running_{false};
    std::thread serverThread_;
    std::unique_ptr<boost::asio::io_context> ioContext_;
};
