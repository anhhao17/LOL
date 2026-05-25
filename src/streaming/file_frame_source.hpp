#pragma once

#include "streaming/i_frame_source.hpp"
#include <atomic>
#include <filesystem>
#include <thread>

namespace streaming {

// Reads an MP4 (or any FFmpeg-supported video) in a background thread,
// encodes each frame as JPEG, and fires the callback at the file's native FPS.
// Loops the file indefinitely. Swap for a CameraFrameSource to go live.
class FileFrameSource final : public IFrameSource {
public:
    explicit FileFrameSource(std::filesystem::path path);
    ~FileFrameSource() override;

    void start(FrameCallback callback) override;
    void stop() noexcept override;

private:
    void loop();

    std::filesystem::path path_;
    FrameCallback         callback_;
    std::atomic<bool>     running_{false};
    std::thread           thread_;
};

} // namespace streaming
