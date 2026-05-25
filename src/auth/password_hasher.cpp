#include "auth/password_hasher.hpp"
#include "common/logger.hpp"

#include <sodium.h>
#include <stdexcept>
#include <array>

namespace auth {

std::string PasswordHasher::hash(const std::string& password) {
    std::array<char, crypto_pwhash_STRBYTES> hashBuf{};

    int rc = crypto_pwhash_str(
        hashBuf.data(),
        password.c_str(), password.size(),
        crypto_pwhash_OPSLIMIT_MODERATE,
        crypto_pwhash_MEMLIMIT_MODERATE);

    if (rc != 0) {
        LOG_ERROR(common::Logger::get("auth"), "Argon2id hash failed (out of memory?)");
        throw std::runtime_error("password hashing failed");
    }

    return std::string(hashBuf.data());
}

bool PasswordHasher::verify(const std::string& password, const std::string& storedHash) noexcept {
    int rc = crypto_pwhash_str_verify(
        storedHash.c_str(),
        password.c_str(), password.size());
    return rc == 0;
}

} // namespace auth
