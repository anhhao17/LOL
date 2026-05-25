#include "streaming/file_frame_source.hpp"
#include "common/logger.hpp"
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace streaming {

static constexpr auto kLog    = "stream";
static constexpr int  kJpegQP = 5;   // 1=best … 31=worst for MJPEG

FileFrameSource::FileFrameSource(std::filesystem::path path)
    : path_(std::move(path)) {}

FileFrameSource::~FileFrameSource() {
    stop();
}

void FileFrameSource::start(FrameCallback callback) {
    callback_ = std::move(callback);
    running_  = true;
    thread_   = std::thread(&FileFrameSource::loop, this);
}

void FileFrameSource::stop() noexcept {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

static void sleepUntil(std::chrono::steady_clock::time_point until,
                       const std::atomic<bool>& running) {
    while (running.load() && std::chrono::steady_clock::now() < until) {
        auto left  = std::chrono::duration_cast<std::chrono::milliseconds>(
                         until - std::chrono::steady_clock::now());
        auto chunk = std::min(left, std::chrono::milliseconds(10));
        if (chunk > std::chrono::nanoseconds(0))
            std::this_thread::sleep_for(chunk);
    }
}

void FileFrameSource::loop() {
    // Route all FFmpeg log output through our own logger; suppress its stderr spam.
    av_log_set_level(AV_LOG_QUIET);

    if (!std::filesystem::exists(path_)) {
        LOG_ERROR(common::Logger::get(kLog), "Video file not found: {}", path_.string());
        return;
    }

    // ── Open container ────────────────────────────────────────────────────────
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path_.c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR(common::Logger::get(kLog), "Cannot open: {}", path_.string());
        return;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        LOG_ERROR(common::Logger::get(kLog), "No stream info in: {}", path_.string());
        return;
    }

    int videoIdx = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIdx = static_cast<int>(i);
            break;
        }
    }
    if (videoIdx < 0) {
        avformat_close_input(&fmtCtx);
        LOG_ERROR(common::Logger::get(kLog), "No video stream in: {}", path_.string());
        return;
    }
    AVStream* stream = fmtCtx->streams[videoIdx];

    // ── Open decoder ──────────────────────────────────────────────────────────
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        avformat_close_input(&fmtCtx);
        LOG_ERROR(common::Logger::get(kLog),
            "No decoder for codec {}", static_cast<int>(stream->codecpar->codec_id));
        return;
    }
    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, stream->codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&fmtCtx);
        LOG_ERROR(common::Logger::get(kLog), "Cannot open decoder");
        return;
    }

    // ── Open MJPEG encoder ────────────────────────────────────────────────────
    // Use YUV420P + AVCOL_RANGE_JPEG instead of the deprecated YUVJ420P format.
    const AVCodec* jpegEnc  = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext* jpegCtx = avcodec_alloc_context3(jpegEnc);
    jpegCtx->width          = decCtx->width;
    jpegCtx->height         = decCtx->height;
    jpegCtx->pix_fmt        = AV_PIX_FMT_YUV420P;
    jpegCtx->color_range    = AVCOL_RANGE_JPEG;
    jpegCtx->time_base      = AVRational{1, 25};
    jpegCtx->flags         |= AV_CODEC_FLAG_QSCALE;
    jpegCtx->global_quality = FF_QP2LAMBDA * kJpegQP;
    if (avcodec_open2(jpegCtx, jpegEnc, nullptr) < 0) {
        avcodec_free_context(&jpegCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&fmtCtx);
        LOG_ERROR(common::Logger::get(kLog), "Cannot open JPEG encoder");
        return;
    }

    // ── Scale context: decoded format → YUV420P (full range for JPEG) ─────────
    SwsContext* swsCtx = sws_getContext(
        decCtx->width, decCtx->height, decCtx->pix_fmt,
        decCtx->width, decCtx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    // Tell swscale the output is full range (JPEG) to avoid the deprecation warning.
    sws_setColorspaceDetails(swsCtx,
        sws_getCoefficients(SWS_CS_DEFAULT), 0,
        sws_getCoefficients(SWS_CS_DEFAULT), 1,  // 1 = full range output
        0, 1 << 16, 1 << 16);

    // ── Allocate frames and packets ───────────────────────────────────────────
    AVFrame* rawFrame = av_frame_alloc();
    AVFrame* yuvFrame = av_frame_alloc();
    av_image_alloc(yuvFrame->data, yuvFrame->linesize,
        decCtx->width, decCtx->height, AV_PIX_FMT_YUV420P, 32);
    yuvFrame->width       = decCtx->width;
    yuvFrame->height      = decCtx->height;
    yuvFrame->format      = AV_PIX_FMT_YUV420P;
    yuvFrame->color_range = AVCOL_RANGE_JPEG;

    AVPacket* pkt     = av_packet_alloc();
    AVPacket* jpegPkt = av_packet_alloc();

    double fps = (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0)
        ? av_q2d(stream->avg_frame_rate)
        : 25.0;
    auto frameDelay = std::chrono::microseconds(static_cast<int64_t>(1e6 / fps));

    LOG_INFO(common::Logger::get(kLog),
        "Streaming '{}' @ {:.1f} fps  {}x{}",
        path_.filename().string(), fps, decCtx->width, decCtx->height);

    auto    nextFrame = std::chrono::steady_clock::now();
    int64_t pts       = 0;  // monotonically increasing PTS for the MJPEG encoder

    // ── Main read/decode loop ─────────────────────────────────────────────────
    while (running_.load()) {
        av_packet_unref(pkt);
        int ret = av_read_frame(fmtCtx, pkt);

        if (ret == AVERROR_EOF) {
            av_seek_frame(fmtCtx, videoIdx, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(decCtx);
            nextFrame = std::chrono::steady_clock::now();
            continue;
        }
        if (ret < 0) {
            LOG_WARN(common::Logger::get(kLog), "av_read_frame error: {}", ret);
            break;
        }
        if (pkt->stream_index != videoIdx) continue;

        if (avcodec_send_packet(decCtx, pkt) < 0) continue;

        while (avcodec_receive_frame(decCtx, rawFrame) >= 0) {
            if (!running_.load()) break;

            sleepUntil(nextFrame, running_);
            nextFrame += frameDelay;

            sws_scale(swsCtx,
                rawFrame->data, rawFrame->linesize, 0, rawFrame->height,
                yuvFrame->data, yuvFrame->linesize);

            yuvFrame->pict_type = AV_PICTURE_TYPE_I;
            yuvFrame->pts       = pts++;
            yuvFrame->quality   = jpegCtx->global_quality;

            if (avcodec_send_frame(jpegCtx, yuvFrame) >= 0 &&
                avcodec_receive_packet(jpegCtx, jpegPkt) >= 0) {
                callback_(std::vector<uint8_t>(
                    jpegPkt->data, jpegPkt->data + jpegPkt->size));
                av_packet_unref(jpegPkt);
            }

            av_frame_unref(rawFrame);
        }
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    av_freep(&yuvFrame->data[0]);
    av_frame_free(&yuvFrame);
    av_frame_free(&rawFrame);
    av_packet_free(&pkt);
    av_packet_free(&jpegPkt);
    sws_freeContext(swsCtx);
    avcodec_free_context(&jpegCtx);
    avcodec_free_context(&decCtx);
    avformat_close_input(&fmtCtx);

    LOG_INFO(common::Logger::get(kLog),
        "Frame source stopped: {}", path_.filename().string());
}

} // namespace streaming
