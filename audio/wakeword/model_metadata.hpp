#pragma once

#include <string>
#include <cstdint>

namespace vani::audio::wakeword {

/**
 * @brief Metadata contract for production wake-word models.
 *
 * Provides runtime introspection of model provenance, acoustic target,
 * versioning, and validation integrity.
 */
struct WakeWordModelMetadata {
    std::string model_name{"unknown"};
    std::string model_version{"0.0.0"};
    std::string wake_word{"VANI"};
    uint32_t sample_rate{16000};
    std::string input_format{"pcm_f32le"};
    std::string model_hash{"none"};
    float calibrated_threshold{0.65f};
    std::string training_dataset_version{"none"};
    bool is_dedicated_model{false}; // true if trained specifically for VANI acoustics, false if generic ASR/KWS
    std::string architecture_type{"generic_kws"}; // "openwakeword_custom", "sherpa_transducer_kws", "custom_cnn"
};

} // namespace vani::audio::wakeword
