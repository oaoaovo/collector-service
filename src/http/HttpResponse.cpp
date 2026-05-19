#include "http/HttpResponse.h"

#include "util/JsonUtil.h"

namespace http_support {

std::string jsonSuccess(const std::string& data) {
    return "{\"success\":true,\"data\":" + data + "}";
}

std::string jsonError(const std::string& message) {
    return "{\"success\":false,\"error\":\"" + util::escapeJson(message) + "\"}";
}

beast_http::response<beast_http::string_body> makeResponse(beast_http::status status,
                                                           const std::string& body,
                                                           unsigned version,
                                                           bool keepAlive) {
    beast_http::response<beast_http::string_body> response{status, version};
    response.set(beast_http::field::server, "collector-service");
    response.set(beast_http::field::content_type, "application/json");
    response.keep_alive(keepAlive);
    response.body() = body;
    response.prepare_payload();
    return response;
}

} // namespace http_support
