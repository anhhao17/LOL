#pragma once

#include <chrono>
#include <string>

namespace session {

enum class Role : unsigned int {
    kAnonymous = 0,
    kViewer    = 1,
    kOperator  = 2,
    kAdmin     = 3,
};

struct Session {
    std::string token;
    std::string csrfToken;
    std::string username;
    Role role{Role::kViewer};
    std::string clientIp;
    std::chrono::steady_clock::time_point expiresAt;

    [[nodiscard]] bool isExpired() const noexcept {
        return std::chrono::steady_clock::now() > expiresAt;
    }

    [[nodiscard]] bool isAdmin() const noexcept { return role == Role::kAdmin; }
};

} // namespace session
