#include "CommandProcessor.h"
#include "DataTask.h"
#include "HttpServer.h"
#include "ModelRepository.h"
#include "MockDriverServer.h"
#include "SQLiteManager.h"
#include "util/Logger.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {

constexpr const char* kMockDriverHost = "127.0.0.1";
constexpr int kMockDriverPort = 9001;

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open command file: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printUsage() {
    Logger::info("usage:");
    Logger::info("  collector-service --console");
    Logger::info("  collector-service --http [port]");
    Logger::info("  collector-service --serve-http [port]");
    Logger::info("  collector-service --cmd <json>");
    Logger::info("  collector-service --cmd-file <path>");
}

bool isExitCommand(const std::string& line) {
    return line == "exit" || line == "quit";
}

bool hasProjectFiles(const std::filesystem::path& path) {
    return std::filesystem::exists(path / "scripts" / "init.sql") &&
           std::filesystem::exists(path / "config");
}

std::filesystem::path resolveProjectRoot(const char* executablePath) {
    const auto cwd = std::filesystem::current_path();
    if (hasProjectFiles(cwd)) {
        return cwd;
    }

    std::filesystem::path exePath(executablePath != nullptr ? executablePath : "");
    if (exePath.is_relative()) {
        exePath = cwd / exePath;
    }
    exePath = std::filesystem::weakly_canonical(exePath);

    auto candidate = exePath.parent_path();
    for (int i = 0; i < 4 && !candidate.empty(); ++i) {
        if (hasProjectFiles(candidate)) {
            return candidate;
        }
        candidate = candidate.parent_path();
    }

    const auto parent = cwd.parent_path();
    if (hasProjectFiles(parent)) {
        return parent;
    }

    return cwd;
}

} // namespace

int main(int argc, char* argv[]) {
    Logger::init();
    Logger::info("collector service started");

    try {
        const auto projectRoot = resolveProjectRoot(argc > 0 ? argv[0] : nullptr);
        const auto databasePath = (projectRoot / "data" / "collector.sqlite").string();
        const auto initSqlPath = (projectRoot / "scripts" / "init.sql").string();
        const auto modelDirectory = (projectRoot / "config" / "models").string();
        const auto fallbackModelDirectory = (projectRoot / "config").string();

        SQLiteManager sqliteManager(databasePath, initSqlPath);
        ModelRepository modelRepository(modelDirectory, fallbackModelDirectory);
        DataTask dataTask(sqliteManager, modelRepository);
        CommandProcessor commandProcessor(sqliteManager, dataTask);

        if (argc >= 2 && std::string(argv[1]) == "--console") {
            MockDriverServer mockDriverServer(kMockDriverHost, kMockDriverPort);
            mockDriverServer.start();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            Logger::info("console mode started. input one-line command JSON, or 'exit' to quit.");
            std::string line;
            while (std::getline(std::cin, line)) {
                if (isExitCommand(line)) {
                    break;
                }
                if (line.empty()) {
                    continue;
                }

                const auto response = commandProcessor.handle(line);
                Logger::info("Command response: " + response);
            }

            dataTask.stopAll();
            mockDriverServer.stop();
            Logger::info("console mode stopped");
            return 0;
        }

        if (argc >= 2 && (std::string(argv[1]) == "--http" || std::string(argv[1]) == "--serve-http")) {
            const int httpPort = argc >= 3 ? std::stoi(argv[2]) : 8080;
            MockDriverServer mockDriverServer(kMockDriverHost, kMockDriverPort);
            mockDriverServer.start();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            HttpServer httpServer("127.0.0.1", httpPort, sqliteManager, dataTask, commandProcessor);
            httpServer.start();
            Logger::info("http mode started. press Enter to stop.");
            std::string stopLine;
            std::getline(std::cin, stopLine);

            dataTask.stopAll();
            httpServer.stop();
            mockDriverServer.stop();
            Logger::info("http mode stopped");
            return 0;
        }

        if (argc >= 3 && (std::string(argv[1]) == "--cmd" || std::string(argv[1]) == "--cmd-file")) {
            MockDriverServer mockDriverServer(kMockDriverHost, kMockDriverPort);
            const std::string commandText = std::string(argv[1]) == "--cmd-file" ? readTextFile(argv[2]) : argv[2];
            const bool startsCollection = commandText.find("start_device_collect") != std::string::npos ||
                                          commandText.find("start_all") != std::string::npos;
            if (startsCollection) {
                mockDriverServer.start();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }

            const auto response = commandProcessor.handle(commandText);
            Logger::info("Command response: " + response);
            if (startsCollection) {
                Logger::info("collection is running. press Enter to stop.");
                std::string stopLine;
                std::getline(std::cin, stopLine);
                dataTask.stopAll();
            }
            mockDriverServer.stop();
            return 0;
        }

        printUsage();
    } catch (const std::exception& ex) {
        Logger::error(std::string("startup failed: ") + ex.what());
        return 1;
    }

    return 0;
}
