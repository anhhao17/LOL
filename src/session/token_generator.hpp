#pragma once

#include <string>

namespace session {

class TokenGenerator {
public:
    TokenGenerator() = default;
    TokenGenerator(const TokenGenerator&) = delete;
    TokenGenerator& operator=(const TokenGenerator&) = delete;

    // Returns a 256-bit (64 hex char) CSPRNG session token.
    [[nodiscard]] static std::string generateSessionToken();

    // Returns a 128-bit (32 hex char) CSPRNG CSRF token.
    [[nodiscard]] static std::string generateCsrfToken();

private:
    [[nodiscard]] static std::string generateHex(size_t byteCount);
};

} // namespace session
