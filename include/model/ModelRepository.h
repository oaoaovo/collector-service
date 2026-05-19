#pragma once

#include <string>

class ModelRepository {
public:
    explicit ModelRepository(std::string modelDirectory = "config/models",
                             std::string fallbackDirectory = "config");

    std::string loadModelText(const std::string& model, const std::string& modelVersion) const;

private:
    std::string loadFile(const std::string& path) const;

    std::string modelDirectory_;
    std::string fallbackDirectory_;
};
