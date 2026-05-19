#pragma once

#include <boost/beast/http.hpp>

class CommandProcessor;
class DataTask;
class SQLiteManager;

namespace http_support {

namespace beast_http = boost::beast::http;

class HttpRouter {
public:
    HttpRouter(SQLiteManager& sqliteManager, DataTask& dataTask, CommandProcessor& commandProcessor);

    beast_http::response<beast_http::string_body> route(
        const beast_http::request<beast_http::string_body>& request);

private:
    SQLiteManager& sqliteManager_;
    DataTask& dataTask_;
    CommandProcessor& commandProcessor_;
};

} // namespace http_support
