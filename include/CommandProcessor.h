#pragma once

#include "Models.h"

#include <string>

class DataTask;
class SQLiteManager;

class CommandProcessor {
public:
    CommandProcessor(SQLiteManager& sqliteManager, DataTask& dataTask);

    std::string handle(const std::string& requestText);

private:
    CommandResult handleCreateDevice(const std::string& requestText, const std::string& mid);
    CommandResult handleDeleteDevice(const std::string& requestText, const std::string& mid);
    CommandResult handleUpdateDevice(const std::string& requestText, const std::string& mid);
    CommandResult handleStartDeviceCollect(const std::string& requestText, const std::string& mid);
    CommandResult handleStopDeviceCollect(const std::string& requestText, const std::string& mid);

    SQLiteManager& sqliteManager_;
    DataTask& dataTask_;
};
