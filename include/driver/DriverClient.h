#pragma once

#include <memory>
#include <string>

class DriverClient {
public:
    DriverClient(std::string host, int port);
    ~DriverClient();

    DriverClient(const DriverClient&) = delete;
    DriverClient& operator=(const DriverClient&) = delete;

    bool connect();
    std::string sendQuery(const std::string& queryText);
    void close();

private:
    struct Impl;

    std::string host_;
    int port_ = 0;
    std::unique_ptr<Impl> impl_;
};
