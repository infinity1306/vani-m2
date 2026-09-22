#pragma once

#include "../common/result.hpp"
#include "../common/cancellation_token.hpp"
#include <string>
#include <vector>
#include <span>
#include <functional>
#include <memory>

namespace vani::contracts {

struct TTSConfig {
    std::string voice_id{"default"};
    float speed{1.0f};
    float pitch{1.0f};
    uint32_t sample_rate_hz{24000};
};

struct TTSCapabilities {
    bool supports_streaming{true};
    bool supports_ssml{false};
    bool is_offline_capable{true};
    uint32_t typical_ttfa_ms{90}; // Time to first audio chunk
    std::vector<std::string> supported_voices{"en_natural", "hi_natural", "hinglish_fluent"};
};

using AudioChunkCallback = std::function<void(std::span<const float> pcm_samples, bool is_final)>;

class TTSEngine {
public:
    virtual ~TTSEngine() = default;

    virtual Result<std::vector<float>> synthesize(
        const std::string& text,
        const TTSConfig& config,
        CancellationToken cancellation_token = CancellationToken::none()
    ) = 0;

    virtual Result<void> synthesize_stream(
        const std::string& text,
        const TTSConfig& config,
        AudioChunkCallback chunk_cb,
        CancellationToken cancellation_token = CancellationToken::none()
    ) = 0;

    [[nodiscard]] virtual std::string engine_name() const = 0;
    [[nodiscard]] virtual TTSCapabilities capabilities() const = 0;
    [[nodiscard]] virtual bool is_healthy() const { return true; }
};

using TTSEnginePtr = std::shared_ptr<TTSEngine>;

} // namespace vani::contracts
