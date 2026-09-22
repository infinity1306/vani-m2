#pragma once

#include "../common/result.hpp"
#include "../common/version.hpp"
#include <string>
#include <vector>
#include <span>
#include <functional>
#include <memory>

namespace vani::contracts {

struct STTConfig {
    uint32_t sample_rate_hz{16000};
    uint8_t channels{1};
    std::string language_preference{"auto"}; // e.g. "auto", "hi", "en", "hinglish"
    bool enable_vad{true};
    bool enable_interim_results{true};
    std::string model_path;
};

struct STTCapabilities {
    bool supports_streaming{true};
    bool supports_interim_results{true};
    bool supports_word_timestamps{false};
    bool supports_multilingual{true};
    bool is_offline_capable{true};
    uint32_t typical_latency_ms{80};
    std::vector<std::string> supported_languages{"en", "hi", "hinglish"};
};

struct STTTranscriptSegment {
    std::string text;
    uint64_t start_ms{0};
    uint64_t end_ms{0};
    float confidence{1.0f};
};

struct STTTranscript {
    std::string text;
    bool is_final{false};
    float confidence{0.0f};
    std::string detected_language{"unknown"};
    uint64_t start_time_ms{0};
    uint64_t end_time_ms{0};
    uint64_t sequence_number{0};
    std::vector<STTTranscriptSegment> segments;
    std::string engine_name;
};

using TranscriptCallback = std::function<void(const STTTranscript& transcript)>;

class STTEngine {
public:
    virtual ~STTEngine() = default;

    virtual Result<void> start_stream(
        const STTConfig& config,
        TranscriptCallback callback
    ) = 0;

    virtual Result<void> push_audio(
        std::span<const float> samples
    ) = 0;

    virtual Result<void> stop_stream() = 0;

    [[nodiscard]] virtual std::string engine_name() const = 0;
    [[nodiscard]] virtual STTCapabilities capabilities() const = 0;
    [[nodiscard]] virtual bool is_healthy() const { return true; }
};

using STTEnginePtr = std::shared_ptr<STTEngine>;

} // namespace vani::contracts
