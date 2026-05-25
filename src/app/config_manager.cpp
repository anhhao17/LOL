#include "app/config_manager.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace app {

void ConfigManager::printHelp(const char* progName) {
    std::cout
        << "Usage: " << progName << " [options]\n"
        << "  --port <n>            Listen port (default: 8080)\n"
        << "  --web-root <dir>      Vue SPA build output (default: ./ui/dist)\n"
        << "  --db <file>           SQLite database path (default: ./quantum.db)\n"
        << "  --log-file <file>     Log file path (default: ./quantum.log)\n"
        << "  --log-level <level>   trace|debug|info|warn|error (default: info)\n"
        << "  --threads <n>         IO thread count (default: 4)\n"
        << "  --max-ws <n>          Max concurrent WS sessions (default: 6)\n"
        << "  --allow-origin <url>  Allowed CORS origin (repeatable)\n"
        << "  --help, -h            Show this help\n";
}

bool ConfigManager::parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + arg);
            return std::string(argv[++i]);
        };

        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return false;
        } else if (arg == "--port") {
            config_.port = static_cast<unsigned short>(std::stoi(next()));
        } else if (arg == "--web-root") {
            config_.webRoot = next();
        } else if (arg == "--db") {
            config_.dbPath = next();
        } else if (arg == "--log-file") {
            config_.logFile = next();
        } else if (arg == "--log-level") {
            config_.logLevel = next();
        } else if (arg == "--threads") {
            config_.threadCount = static_cast<size_t>(std::stoi(next()));
        } else if (arg == "--max-ws") {
            config_.maxWsSessions = static_cast<size_t>(std::stoi(next()));
        } else if (arg == "--allow-origin") {
            config_.allowedOrigins.push_back(next());
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }
    return true;
}

} // namespace app
