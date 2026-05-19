#include "util/Logger.h"

#if defined(COLLECTOR_USE_SPDLOG)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#else
#include <iostream>
#endif

void Logger::init() {
#if defined(COLLECTOR_USE_SPDLOG)
    auto logger = spdlog::stdout_color_mt("collector-service");
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
#endif
}

void Logger::info(const std::string& message) {
#if defined(COLLECTOR_USE_SPDLOG)
    spdlog::info(message);
#else
    std::cout << "[info] " << message << '\n';
#endif
}

void Logger::warn(const std::string& message) {
#if defined(COLLECTOR_USE_SPDLOG)
    spdlog::warn(message);
#else
    std::cout << "[warn] " << message << '\n';
#endif
}

void Logger::error(const std::string& message) {
#if defined(COLLECTOR_USE_SPDLOG)
    spdlog::error(message);
#else
    std::cerr << "[error] " << message << '\n';
#endif
}
