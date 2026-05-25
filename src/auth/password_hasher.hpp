#pragma once

#include <string>

namespace auth {

class PasswordHasher {
public:
    PasswordHasher() = default;
    PasswordHasher(const PasswordHasher&) = delete;
    PasswordHasher& operator=(const PasswordHasher&) = delete;

    // Returns Argon2id hash string. Throws on failure.
    [[nodiscard]] static std::string hash(const std::string& password);

    // Constant-time verification. Returns false on mismatch or error.
    [[nodiscard]] static bool verify(const std::string& password, const std::string& hash) noexcept;
};

} // namespace auth
