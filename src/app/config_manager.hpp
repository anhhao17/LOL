#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace app {

struct Config {
    unsigned short port{8080};
    std::filesystem::path webRoot{"./ui/dist"};
    std::filesystem::path dbPath{"./quantum.db"};
    std::filesystem::path logFile{"./quantum.log"};
    std::string logLevel{"info"};
    size_t threadCount{4};
    size_t maxWsSessions{6};
    unsigned int maxRequestsPerWindow{200};
    std::chrono::seconds rateWindow{10};
    std::vector<std::string> allowedOrigins;
    std::filesystem::path videoFileCl;  // MP4 for CL channel (optional)
    std::filesystem::path videoFileIr;  // MP4 for IR channel (optional)
};

class ConfigManager {
public:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Parses CLI args. Returns false if --help was requested.
    [[nodiscard]] bool parse(int argc, char** argv);

    [[nodiscard]] const Config& config() const noexcept { return config_; }

private:
    Config config_;
    static void printHelp(const char* progName);
};

} // namespace app
