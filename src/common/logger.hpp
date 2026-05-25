#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

namespace common {

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    static void init(const std::string& logFile, spdlog::level::level_enum level = spdlog::level::info);
    static void shutdown() noexcept;

    [[nodiscard]] static std::shared_ptr<spdlog::logger> get(const std::string& name);

private:
    Logger() = default;
};

} // namespace common

#define LOG_TRACE(logger, ...)   (logger)->trace(__VA_ARGS__)
#define LOG_DEBUG(logger, ...)   (logger)->debug(__VA_ARGS__)
#define LOG_INFO(logger, ...)    (logger)->info(__VA_ARGS__)
#define LOG_WARN(logger, ...)    (logger)->warn(__VA_ARGS__)
#define LOG_ERROR(logger, ...)   (logger)->error(__VA_ARGS__)
#define LOG_CRITICAL(logger, ...) (logger)->critical(__VA_ARGS__)
