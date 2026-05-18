#include "ModelRepository.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

bool hasDataItemText(const std::string& value) {
    return value.find("DataItem:") != std::string::npos;
}

} // namespace

ModelRepository::ModelRepository(std::string modelDirectory, std::string fallbackDirectory)
    : modelDirectory_(std::move(modelDirectory)), fallbackDirectory_(std::move(fallbackDirectory)) {}

std::string ModelRepository::loadModelText(const std::string& model, const std::string& modelVersion) const {
    if (model.empty()) {
        throw std::runtime_error("model is empty");
    }
    if (hasDataItemText(model)) {
        return model;
    }

    const std::filesystem::path modelDirectory(modelDirectory_);
    const std::filesystem::path fallbackDirectory(fallbackDirectory_);
    std::vector<std::filesystem::path> candidates;

    if (!modelVersion.empty()) {
        candidates.push_back(modelDirectory / (model + "-" + modelVersion + ".yaml"));
        candidates.push_back(modelDirectory / (model + "-" + modelVersion + ".yml"));
        candidates.push_back(modelDirectory / (model + "-" + modelVersion + ".txt"));
    }
    candidates.push_back(modelDirectory / (model + ".yaml"));
    candidates.push_back(modelDirectory / (model + ".yml"));
    candidates.push_back(modelDirectory / (model + ".txt"));

    if (model == "M1") {
        candidates.push_back(fallbackDirectory / "model.txt");
    }

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return loadFile(candidate.string());
        }
    }

    throw std::runtime_error("model definition not found: " + model);
}

std::string ModelRepository::loadFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open model file: " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
