#pragma once

#include <boost/beast/http.hpp>

#include <string>

namespace http_support {

namespace beast_http = boost::beast::http;

std::string jsonSuccess(const std::string& data);
std::string jsonError(const std::string& message);

beast_http::response<beast_http::string_body> makeResponse(beast_http::status status,
                                                           const std::string& body,
                                                           unsigned version,
                                                           bool keepAlive);

} // namespace http_support
