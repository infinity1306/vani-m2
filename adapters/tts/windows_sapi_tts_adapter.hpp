#pragma once

#include "../../contracts/providers/tts_engine.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace vani::adapters::tts {

struct SAPIConfig {
    std::string voice_name{"Default System Voice"};
    float speed{1.0f};
    float pitch{1.0f};
    uint32_t sample_rate{22050};
};

class WindowsSAPITTSAdapter : public contracts::TTSEngine {
public:
    explicit WindowsSAPITTSAdapter(const SAPIConfig& config = {});
    ~WindowsSAPITTSAdapter() override;

    WindowsSAPITTSAdapter(const WindowsSAPITTSAdapter&) = delete;
    WindowsSAPITTSAdapter& operator=(const WindowsSAPITTSAdapter&) = delete;
    WindowsSAPITTSAdapter(WindowsSAPITTSAdapter&&) noexcept;
    WindowsSAPITTSAdapter& operator=(WindowsSAPITTSAdapter&&) noexcept;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& config = {},
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        const contracts::TTSConfig& config,
        contracts::AudioChunkCallback chunk_cb,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] contracts::TTSCapabilities capabilities() const override;
    [[nodiscard]] bool is_healthy() const override;

private:
    SAPIConfig config_;
    bool initialized_{true};
    mutable std::mutex mutex_;
};

} // namespace vani::adapters::tts
