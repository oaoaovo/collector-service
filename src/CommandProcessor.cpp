#include "CommandProcessor.h"

#include "DataTask.h"
#include "SQLiteManager.h"
#include "command/CommandJsonSerializer.h"
#include "command/CommandParser.h"
#include "util/Logger.h"

#include <stdexcept>

namespace {

CommandResult errorResult(const std::string& mid, const std::string& msg) {
    CommandResult result;
    result.code = 500;
    result.msg = msg;
    result.mid = mid;
    return result;
}

} // namespace

CommandProcessor::CommandProcessor(SQLiteManager& sqliteManager, DataTask& dataTask)
    : sqliteManager_(sqliteManager), dataTask_(dataTask) {}

std::string CommandProcessor::handle(const std::string& requestText) {
    const auto mid = command::extractStringField(requestText, "mid");
    const auto category = command::extractStringField(requestText, "category");

    try {
        if (category == "create_device") {
            return command::toJson(handleCreateDevice(requestText, mid));
        }
        if (category == "delete_device") {
            return command::toJson(handleDeleteDevice(requestText, mid));
        }
        if (category == "update_device") {
            return command::toJson(handleUpdateDevice(requestText, mid));
        }
        if (category == "start_device_collect") {
            return command::toJson(handleStartDeviceCollect(requestText, mid));
        }
        if (category == "stop_device_collect") {
            return command::toJson(handleStopDeviceCollect(requestText, mid));
        }
        if (category == "start_all") {
            return command::toJson(handleStartAll(mid));
        }
        if (category == "stop_all") {
            return command::toJson(handleStopAll(mid));
        }
        if (category == "get_status") {
            return command::toJson(dataTask_.getStatus(), mid);
        }
        return command::toJson(errorResult(mid, "unknown cmd category: " + category));
    } catch (const std::exception& ex) {
        return command::toJson(errorResult(mid, ex.what()));
    }
}

CommandResult CommandProcessor::handleCreateDevice(const std::string& requestText, const std::string& mid) {
    auto device = command::parseDeviceUpsert(requestText);
    if (device.name.empty()) {
        return errorResult(mid, "device_name is empty");
    }
    if (device.model.empty()) {
        return errorResult(mid, "model is empty");
    }
    if (sqliteManager_.deviceExists(device.name)) {
        return errorResult(mid, "device already exists: " + device.name);
    }

    sqliteManager_.createDevice(device);
    const auto initResult = dataTask_.initDataPointsByDevice(device.name, mid);
    if (!initResult.ok()) {
        Logger::warn("create_device created device but failed to initialize DataPoint: " + initResult.msg);
        return initResult;
    }

    CommandResult result;
    result.mid = mid;
    result.msg = "device created: " + device.name;
    return result;
}

CommandResult CommandProcessor::handleDeleteDevice(const std::string& requestText, const std::string& mid) {
    const auto deviceName = command::extractStringField(requestText, "device_name");
    if (!sqliteManager_.deviceExists(deviceName)) {
        return errorResult(mid, "device not found: " + deviceName);
    }

    sqliteManager_.deleteDevice(deviceName);
    CommandResult result;
    result.mid = mid;
    result.msg = "device deleted: " + deviceName;
    return result;
}

CommandResult CommandProcessor::handleUpdateDevice(const std::string& requestText, const std::string& mid) {
    auto device = command::parseDeviceUpsert(requestText);
    if (device.name.empty()) {
        return errorResult(mid, "device_name is empty");
    }
    if (device.model.empty()) {
        return errorResult(mid, "model is empty");
    }
    if (!sqliteManager_.deviceExists(device.oldName.empty() ? device.name : device.oldName)) {
        return errorResult(mid, "device not found: " + device.name);
    }

    const auto before = sqliteManager_.getDeviceByName(device.oldName.empty() ? device.name : device.oldName);
    if (!command::hasField(requestText, "status") && !command::hasField(requestText, "Status")) {
        device.status = before.status;
    }
    sqliteManager_.updateDevice(device);
    if (before.model != device.model || before.modelVersion != device.modelVersion) {
        const auto initResult = dataTask_.initDataPointsByDevice(device.name, mid);
        if (!initResult.ok()) {
            return initResult;
        }
    }

    CommandResult result;
    result.mid = mid;
    result.msg = "device updated: " + device.name;
    return result;
}

CommandResult CommandProcessor::handleStartDeviceCollect(const std::string& requestText, const std::string& mid) {
    return dataTask_.startDeviceCollect(command::extractStringField(requestText, "device_name"), mid);
}

CommandResult CommandProcessor::handleStopDeviceCollect(const std::string& requestText, const std::string& mid) {
    return dataTask_.stopDeviceCollect(command::extractStringField(requestText, "device_name"), mid);
}

CommandResult CommandProcessor::handleStartAll(const std::string& mid) {
    dataTask_.startAll();

    CommandResult result;
    result.mid = mid;
    result.msg = "all online device collections started";
    return result;
}

CommandResult CommandProcessor::handleStopAll(const std::string& mid) {
    dataTask_.stopAll();

    CommandResult result;
    result.mid = mid;
    result.msg = "all device collections stopped";
    return result;
}
