#pragma once

#include "../../audio/wakeword/wakeword_engine.hpp"
#include "../../audio/wakeword/model_metadata.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vani::adapters::wakeword {

/**
 * @brief Dedicated Neural Wake-Word Adapter for the exact phrase "VANI".
 *
 * Implements a dedicated acoustic wake-word classifier with explicit metadata,
 * sensitivity thresholding, windowed feature buffer, and acoustic score calibration.
 */
class DedicatedVaniWakeWordAdapter : public audio::wakeword::WakeWordEngine {
public:
    struct Options {
        std::string model_path{"models/wakeword/vani_dedicated.onnx"};
        std::string model_version{"1.0.0-phase6b"};
        std::string training_dataset_version{"vani-corpus-v1.0"};
        float detection_threshold{0.65f};
        uint32_t sample_rate{16000};
        uint32_t window_size_samples{16000}; // 1.0s context window
        uint32_t hop_size_samples{480};      // 30ms step
        int32_t num_threads{1};
    };

    DedicatedVaniWakeWordAdapter();
    explicit DedicatedVaniWakeWordAdapter(const Options& options, audio::wakeword::WakeWordConfig config = {});
    ~DedicatedVaniWakeWordAdapter() override;

    // Non-copyable, movable
    DedicatedVaniWakeWordAdapter(const DedicatedVaniWakeWordAdapter&) = delete;
    DedicatedVaniWakeWordAdapter& operator=(const DedicatedVaniWakeWordAdapter&) = delete;
    DedicatedVaniWakeWordAdapter(DedicatedVaniWakeWordAdapter&&) noexcept;
    DedicatedVaniWakeWordAdapter& operator=(DedicatedVaniWakeWordAdapter&&) noexcept;

    // audio::wakeword::WakeWordEngine interface
    audio::wakeword::DetectionResult process(std::span<const float> audio_frame) override;
    void reset() override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] const audio::wakeword::WakeWordConfig& config() const override;
    void set_config(const audio::wakeword::WakeWordConfig& config) override;
    void set_enabled(bool enabled) override;
    [[nodiscard]] bool is_enabled() const override;
    [[nodiscard]] bool is_healthy() const override;
    [[nodiscard]] audio::wakeword::WakeWordModelMetadata metadata() const override;

    // Test & Calibration harness support
    void calibrate_threshold(float threshold);
    void trigger_simulated_detection(const std::string& word = "VANI", float confidence = 0.98f);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vani::adapters::wakeword
