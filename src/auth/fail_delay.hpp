#pragma once

#include "common/error.hpp"
#include <chrono>
#include <map>
#include <mutex>
#include <string>

namespace auth {

static constexpr int kMaxFailAttempts = 5;
static constexpr auto kLockoutDuration = std::chrono::minutes(5);

class FailDelay {
public:
    FailDelay() = default;
    FailDelay(const FailDelay&) = delete;
    FailDelay& operator=(const FailDelay&) = delete;

    // Returns kOk if not locked out, kTooManyRequests if locked.
    [[nodiscard]] common::ErrorCode check(const std::string& ip) const noexcept;

    void recordFailure(const std::string& ip) noexcept;
    void recordSuccess(const std::string& ip) noexcept;
    void purgeExpired() noexcept;

private:
    struct Entry {
        int failCount{0};
        std::chrono::steady_clock::time_point lockedUntil{};
    };

    std::map<std::string, Entry> entries_;
    mutable std::mutex mutex_;
};

} // namespace auth
