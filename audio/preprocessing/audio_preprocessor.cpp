#include "audio_preprocessor.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace vani::audio::preprocessing {

AudioPreprocessor::AudioPreprocessor() {
    add_stage(std::make_shared<SimpleNoiseSuppressionStage>());
    add_stage(std::make_shared<GainNormalizationStage>());
}

void AudioPreprocessor::add_stage(PreprocessorStagePtr stage) {
    if (!stage) return;
    std::lock_guard<std::mutex> lock(mutex_);
    stages_.push_back(std::move(stage));
}

void AudioPreprocessor::clear_stages() {
    std::lock_guard<std::mutex> lock(mutex_);
    stages_.clear();
}

size_t AudioPreprocessor::stage_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stages_.size();
}

std::vector<float> AudioPreprocessor::process_chain(
    std::span<const float> input,
    const AudioFormat& format
) {
    if (input.empty()) return {};

    std::vector<float> current(input.begin(), input.end());

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& stage : stages_) {
        if (stage && stage->is_enabled()) {
            current = stage->process(current, format);
        }
    }

    return current;
}

// GainNormalizationStage Implementation
std::vector<float> GainNormalizationStage::process(
    std::span<const float> input,
    const AudioFormat& /*format*/
) {
    if (input.empty()) return {};

    // Calculate RMS energy
    double sum_sq = 0.0;
    for (float s : input) {
        sum_sq += static_cast<double>(s) * static_cast<double>(s);
    }
    double current_rms = std::sqrt(sum_sq / static_cast<double>(input.size()));

    if (current_rms < 1e-6) {
        return std::vector<float>(input.begin(), input.end());
    }

    double gain = target_rms_ / current_rms;
    // Clamp maximum amplification to 12dB (approx 4.0x) to avoid blowing up noise floor
    gain = std::clamp(gain, 0.2, 4.0);

    std::vector<float> normalized(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        float val = static_cast<float>(input[i] * gain);
        normalized[i] = std::clamp(val, -1.0f, 1.0f);
    }
    return normalized;
}

// SimpleNoiseSuppressionStage Implementation
std::vector<float> SimpleNoiseSuppressionStage::process(
    std::span<const float> input,
    const AudioFormat& /*format*/
) {
    if (input.empty()) return {};

    std::vector<float> cleaned(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        float sample = input[i];
        if (std::abs(sample) < threshold_) {
            cleaned[i] = 0.0f; // Gate low-level noise
        } else {
            cleaned[i] = sample;
        }
    }
    return cleaned;
}

} // namespace vani::audio::preprocessing
