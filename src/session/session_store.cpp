#include "session/session_store.hpp"
#include "session/token_generator.hpp"
#include "common/logger.hpp"

namespace session {

Session SessionStore::create(const std::string& username, Role role, const std::string& clientIp) {
    Session s;
    s.token     = TokenGenerator::generateSessionToken();
    s.csrfToken = TokenGenerator::generateCsrfToken();
    s.username  = username;
    s.role      = role;
    s.clientIp  = clientIp;
    s.expiresAt = std::chrono::steady_clock::now() + kSessionTtl;

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.emplace(s.token, s);

    LOG_INFO(common::Logger::get("session"), "Session created for user='{}' ip='{}'", username, clientIp);
    return s;
}

std::optional<Session> SessionStore::validate(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    if (it->second.isExpired()) {
        sessions_.erase(it);
        return std::nullopt;
    }

    // Slide expiry window on active use.
    it->second.expiresAt = std::chrono::steady_clock::now() + kSessionTtl;
    return it->second;
}

void SessionStore::revoke(const std::string& token) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        LOG_INFO(common::Logger::get("session"), "Session revoked for user='{}'", it->second.username);
        sessions_.erase(it);
    }
}

void SessionStore::revokeUser(const std::string& username) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.username == username) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void SessionStore::purgeExpired() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.isExpired()) {
            it = sessions_.erase(it);
            ++count;
        } else {
            ++it;
        }
    }
    if (count > 0) {
        LOG_DEBUG(common::Logger::get("session"), "Purged {} expired sessions", count);
    }
}

size_t SessionStore::count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

} // namespace session
