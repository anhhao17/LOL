#include "common/logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/registry.h>
#include <vector>

namespace common {

void Logger::init(const std::string& logFile, spdlog::level::level_enum level) {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(level);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v");

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFile, 10 * 1024 * 1024, 3);
    fileSink->set_level(level);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%s:%#] %v");

    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};

    static const std::vector<std::string> kLoggerNames{
        "app", "server", "router", "auth", "session",
        "middleware", "api", "stream", "websocket"
    };

    for (const auto& name : kLoggerNames) {
        auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(level);
        logger->flush_on(spdlog::level::warn);
        spdlog::register_logger(logger);
    }

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end()));
    spdlog::set_level(level);
}

void Logger::shutdown() noexcept {
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> Logger::get(const std::string& name) {
    auto logger = spdlog::get(name);
    if (!logger) {
        logger = spdlog::default_logger();
    }
    return logger;
}

} // namespace common
