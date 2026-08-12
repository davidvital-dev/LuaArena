#pragma once

#include <string>
#include <vector>

struct DifficultyConfig {
    double healthMultiplier = 1.0;
    double attackMultiplier = 1.0;
    double criticalChance = 0.10;
    bool healingEnabled = true;
};

class DifficultyLoader {
public:
    bool load(const std::string& scriptPath);

    const DifficultyConfig& config() const noexcept;
    const std::string& lastError() const noexcept;
    const std::vector<std::string>& warnings() const noexcept;

private:
    DifficultyConfig config_;
    std::string lastError_;
    std::vector<std::string> warnings_;
};
