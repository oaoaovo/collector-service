cmake_minimum_required(VERSION 3.16)

project(collector_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(collector-service
    src/main.cpp
    src/util/Logger.cpp
    src/util/JsonUtil.cpp
    src/util/TimeUtil.cpp
    src/db/SQLiteConnection.cpp
    src/db/SQLiteStatement.cpp
    src/db/SQLiteTransaction.cpp
    src/db/DeviceRepository.cpp
    src/db/DataPointRepository.cpp
    src/db/ResourceRepository.cpp
    src/db/ResourceRetentionService.cpp
    src/SQLiteManager.cpp
    src/MockDriverServer.cpp
    src/DriverClient.cpp
    src/DataTask.cpp
    src/ModelParser.cpp
    src/ModelRepository.cpp
    src/CommandProcessor.cpp
    src/command/CommandParser.cpp
    src/command/CommandJsonSerializer.cpp
    src/http/HttpResponse.cpp
    src/http/HttpJsonSerializer.cpp
    src/http/HttpCommandAdapter.cpp
    src/http/HttpRouter.cpp
    src/task/CollectionQueryBuilder.cpp
    src/task/CollectionResponseParser.cpp
    src/task/DeviceStatusStore.cpp
    src/HttpServer.cpp
)

target_include_directories(collector-service
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(spdlog CONFIG QUIET)
if (spdlog_FOUND)
    target_link_libraries(collector-service PRIVATE spdlog::spdlog)
    target_compile_definitions(collector-service PRIVATE COLLECTOR_USE_SPDLOG)
endif()

find_package(SQLite3 REQUIRED)
target_link_libraries(collector-service PRIVATE SQLite::SQLite3)

find_package(Boost QUIET)
if (Boost_FOUND)
    target_include_directories(collector-service PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if (WIN32)
    target_link_libraries(collector-service PRIVATE ws2_32 mswsock)
endif()

if (MSVC)
    target_compile_options(collector-service PRIVATE /W4)
else()
    target_compile_options(collector-service PRIVATE -Wall -Wextra -Wpedantic)
endif()
