#pragma once

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <thread>

class CommandProcessor;
class DataTask;
class SQLiteManager;

namespace boost {
namespace asio {
class io_context;
} // namespace asio
} // namespace boost

class HttpServer {
public:
    HttpServer(std::string host,
               int port,
               SQLiteManager& sqliteManager,
               DataTask& dataTask,
               CommandProcessor& commandProcessor);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void start();
    void stop();

private:
    void run();
    void handleSession(boost::asio::ip::tcp::socket socket);

    std::string host_;
    int port_ = 0;
    SQLiteManager& sqliteManager_;
    DataTask& dataTask_;
    CommandProcessor& commandProcessor_;
    std::atomic_bool running_{false};
    std::thread serverThread_;
    std::unique_ptr<boost::asio::io_context> ioContext_;
};
