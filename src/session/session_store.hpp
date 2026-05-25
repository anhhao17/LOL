#pragma once

#include "session/session.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace session {

static constexpr auto kSessionTtl = std::chrono::minutes(30);

class SessionStore {
public:
    SessionStore() = default;
    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

    // Creates and stores a new session. Returns the new Session.
    [[nodiscard]] Session create(const std::string& username, Role role, const std::string& clientIp);

    // Returns the session if found and not expired; slides the expiry window.
    [[nodiscard]] std::optional<Session> validate(const std::string& token);

    // Removes session by token.
    void revoke(const std::string& token) noexcept;

    // Removes all sessions belonging to a user.
    void revokeUser(const std::string& username) noexcept;

    // Purges all expired sessions. Called periodically.
    void purgeExpired() noexcept;

    [[nodiscard]] size_t count() const noexcept;

private:
    std::map<std::string, Session> sessions_;
    mutable std::mutex mutex_;
};

} // namespace session
