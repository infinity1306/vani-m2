#pragma once

#include "../../audio/wakeword/wakeword_engine.hpp"
#include <memory>
#include <string>

namespace vani::adapters::wakeword {

/**
 * @brief Real Sherpa-ONNX Keyword Spotter Adapter for continuous low-power Wake-Word detection.
 *
 * Implements offline streaming keyword detection for "vani" / "hey vani" using
 * the Sherpa-ONNX KeywordSpotter C-API.
 */
class RealSherpaKwsAdapter : public audio::wakeword::WakeWordEngine {
public:
    struct Options {
        std::string encoder_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/encoder-epoch-99-avg-1.int8.onnx"};
        std::string decoder_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/decoder-epoch-99-avg-1.int8.onnx"};
        std::string joiner_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/joiner-epoch-99-avg-1.onnx"};
        std::string tokens_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/tokens.txt"};
        // Valid SentencePiece BPE keyword representations for VANI (U+2581 = \xe2\x96\x81)
        // \xe2\x96\x81VA (345) N (13) I (27)
        // \xe2\x96\x81 (34) V (50) A (20) N (13) I (27)
        // V (50) A (20) N (13) I (27)
        // \xe2\x96\x81HE (22) Y (16) \xe2\x96\x81VA (345) N (13) I (27)
        // \xe2\x96\x81HE (22) Y (16) \xe2\x96\x81 V A N I (34, 50, 20, 13, 27)
        std::string keywords{
            "\xe2\x96\x81VA N I :2.5 #0.25 @vani\n"
            "\xe2\x96\x81 V A N I :2.5 #0.25 @vani\n"
            "V A N I :2.5 #0.25 @vani\n"
            "\xe2\x96\x81HE Y \xe2\x96\x81VA N I :2.5 #0.25 @hey_vani\n"
            "\xe2\x96\x81HE Y \xe2\x96\x81 V A N I :2.5 #0.25 @hey_vani"
        };
        float keywords_score{2.5f};
        float keywords_threshold{0.25f};
        int32_t num_threads{1};
        std::string provider{"cpu"};
    };

    RealSherpaKwsAdapter();
    explicit RealSherpaKwsAdapter(const Options& options, audio::wakeword::WakeWordConfig config = {});
    ~RealSherpaKwsAdapter() override;

    // Non-copyable, movable
    RealSherpaKwsAdapter(const RealSherpaKwsAdapter&) = delete;
    RealSherpaKwsAdapter& operator=(const RealSherpaKwsAdapter&) = delete;
    RealSherpaKwsAdapter(RealSherpaKwsAdapter&&) noexcept;
    RealSherpaKwsAdapter& operator=(RealSherpaKwsAdapter&&) noexcept;

    // audio::wakeword::WakeWordEngine interface
    audio::wakeword::DetectionResult process(std::span<const float> audio_frame) override;
    void reset() override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] const audio::wakeword::WakeWordConfig& config() const override;
    void set_config(const audio::wakeword::WakeWordConfig& config) override;
    void set_enabled(bool enabled) override;
    [[nodiscard]] bool is_enabled() const override;
    [[nodiscard]] bool is_healthy() const;
    [[nodiscard]] uint64_t decode_count() const;
    [[nodiscard]] uint64_t wake_detection_count() const;
    [[nodiscard]] float max_observed_score() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vani::adapters::wakeword
