#include "session/token_generator.hpp"

#include <sodium.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>

namespace session {

std::string TokenGenerator::generateHex(size_t byteCount) {
    std::vector<unsigned char> buf(byteCount);
    randombytes_buf(buf.data(), byteCount);

    std::ostringstream oss;
    for (auto byte : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::string TokenGenerator::generateSessionToken() {
    return generateHex(32); // 256-bit
}

std::string TokenGenerator::generateCsrfToken() {
    return generateHex(16); // 128-bit
}

} // namespace session
