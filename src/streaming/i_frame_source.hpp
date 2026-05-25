#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace streaming {

enum class SourceChannel : uint8_t {
    CL = 0x00,
    IR = 0x01,
};

// Each frame is a raw JPEG payload (no header byte — caller adds it).
using FrameCallback = std::function<void(const std::vector<uint8_t>& jpegFrame)>;

class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual void start(FrameCallback callback) = 0;
    virtual void stop() noexcept = 0;
};

} // namespace streaming
