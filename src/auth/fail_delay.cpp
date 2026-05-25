#include "auth/fail_delay.hpp"
#include "common/logger.hpp"

namespace auth {

common::ErrorCode FailDelay::check(const std::string& ip) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(ip);
    if (it == entries_.end()) {
        return common::ErrorCode::kOk;
    }
    if (std::chrono::steady_clock::now() < it->second.lockedUntil) {
        LOG_WARN(common::Logger::get("auth"), "Login blocked (lockout active) for ip='{}'", ip);
        return common::ErrorCode::kTooManyRequests;
    }
    return common::ErrorCode::kOk;
}

void FailDelay::recordFailure(const std::string& ip) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = entries_[ip];
    entry.failCount++;
    if (entry.failCount >= kMaxFailAttempts) {
        entry.lockedUntil = std::chrono::steady_clock::now() + kLockoutDuration;
        LOG_WARN(common::Logger::get("auth"),
            "IP '{}' locked out after {} failed login attempts", ip, entry.failCount);
    }
}

void FailDelay::recordSuccess(const std::string& ip) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(ip);
}

void FailDelay::purgeExpired() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.lockedUntil < now && it->second.failCount > 0) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace auth
