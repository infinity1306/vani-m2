#include "real_silero_vad_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>
#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace vani::adapters::vad {

struct RealSileroVADAdapter::Impl {
    const SherpaOnnxVoiceActivityDetector* vad{nullptr};
    RealSileroVADAdapter::Options options;
    bool was_speaking{false};
    uint32_t consecutive_speech_frames{0};
    uint32_t consecutive_silence_frames{0};
    uint64_t frame_count{0};
    std::vector<float> sample_accumulator;

    Impl(const RealSileroVADAdapter::Options& opt) : options(opt) {
        SherpaOnnxVadModelConfig vad_config;
        std::memset(&vad_config, 0, sizeof(vad_config));
        vad_config.silero_vad.model = options.model_path.c_str();
        vad_config.silero_vad.threshold = options.threshold;
        vad_config.silero_vad.min_silence_duration = options.min_silence_duration_s;
        vad_config.silero_vad.min_speech_duration = options.min_speech_duration_s;
        vad_config.silero_vad.window_size = options.window_size;
        vad_config.silero_vad.max_speech_duration = 20.0f;
        vad_config.sample_rate = options.sample_rate;
        vad_config.num_threads = options.num_threads;
        vad_config.provider = options.provider.c_str();
        vad_config.debug = 0;

        vad = SherpaOnnxCreateVoiceActivityDetector(&vad_config, 10.0f);
    }

    ~Impl() {
        if (vad) {
            SherpaOnnxDestroyVoiceActivityDetector(vad);
            vad = nullptr;
        }
    }
};

RealSileroVADAdapter::RealSileroVADAdapter()
    : RealSileroVADAdapter(Options{}) {}

RealSileroVADAdapter::RealSileroVADAdapter(const Options& options)
    : impl_(std::make_unique<Impl>(options)) {
    config_.positive_speech_threshold = options.threshold;
    config_.negative_speech_threshold = options.threshold * 0.7f;
    config_.min_speech_duration_ms = static_cast<uint32_t>(options.min_speech_duration_s * 1000.0f);
    config_.min_silence_duration_ms = static_cast<uint32_t>(options.min_silence_duration_s * 1000.0f);
    config_.sample_rate = static_cast<uint32_t>(options.sample_rate);
}

RealSileroVADAdapter::~RealSileroVADAdapter() = default;
RealSileroVADAdapter::RealSileroVADAdapter(RealSileroVADAdapter&&) noexcept = default;
RealSileroVADAdapter& RealSileroVADAdapter::operator=(RealSileroVADAdapter&&) noexcept = default;

bool RealSileroVADAdapter::is_model_loaded() const {
    return impl_ && impl_->vad != nullptr;
}

audio::vad::VADResult RealSileroVADAdapter::process(std::span<const float> audio_frame) {
    if (audio_frame.empty()) {
        return {
            .state = audio::vad::VADState::Silence,
            .speech_probability = 0.0f,
            .timestamp_ms = 0,
            .speech_duration_ms = 0,
            .silence_duration_ms = 0,
            .is_speech = false
        };
    }

    impl_->frame_count++;

    // Calculate frame RMS for probability estimate
    double sum_sq = 0.0;
    for (float s : audio_frame) {
        sum_sq += static_cast<double>(s) * static_cast<double>(s);
    }
    double rms = std::sqrt(sum_sq / static_cast<double>(audio_frame.size()));
    float probability = static_cast<float>(std::clamp(rms * 10.0, 0.0, 1.0));

    bool is_speech = false;

    if (impl_->vad) {
        // Feed into real Silero neural VAD
        SherpaOnnxVoiceActivityDetectorAcceptWaveform(
            impl_->vad,
            audio_frame.data(),
            static_cast<int32_t>(audio_frame.size())
        );

        int32_t detected = SherpaOnnxVoiceActivityDetectorDetected(impl_->vad);
        is_speech = (detected != 0);

        if (is_speech) {
            probability = std::max(probability, 0.85f);
        } else if (probability > 0.6f) {
            probability = 0.45f;
        }
    } else {
        // Fallback energy threshold
        is_speech = probability >= config_.positive_speech_threshold;
    }

    audio::vad::VADState state = audio::vad::VADState::Silence;

    if (is_speech) {
        if (!impl_->was_speaking) {
            state = audio::vad::VADState::SpeechStarted;
            impl_->was_speaking = true;
        } else {
            state = audio::vad::VADState::SpeechContinuing;
        }
        impl_->consecutive_silence_frames = 0;
        impl_->consecutive_speech_frames++;
    } else {
        if (impl_->was_speaking) {
            impl_->consecutive_silence_frames++;
            uint32_t silence_ms = (impl_->consecutive_silence_frames * static_cast<uint32_t>(audio_frame.size()) * 1000) / config_.sample_rate;
            if (silence_ms >= config_.min_silence_duration_ms) {
                state = audio::vad::VADState::SpeechEnded;
                impl_->was_speaking = false;
                impl_->consecutive_speech_frames = 0;
            } else {
                state = audio::vad::VADState::SpeechContinuing;
            }
        } else {
            state = audio::vad::VADState::Silence;
        }
    }

    uint32_t frame_duration_ms = (static_cast<uint32_t>(audio_frame.size()) * 1000) / config_.sample_rate;
    uint64_t timestamp_ms = (impl_->frame_count * static_cast<uint64_t>(audio_frame.size()) * 1000) / config_.sample_rate;

    return {
        .state = state,
        .speech_probability = probability,
        .timestamp_ms = timestamp_ms,
        .speech_duration_ms = impl_->consecutive_speech_frames * frame_duration_ms,
        .silence_duration_ms = impl_->consecutive_silence_frames * frame_duration_ms,
        .is_speech = impl_->was_speaking || is_speech
    };
}

void RealSileroVADAdapter::reset() {
    if (impl_) {
        if (impl_->vad) {
            SherpaOnnxVoiceActivityDetectorReset(impl_->vad);
        }
        impl_->was_speaking = false;
        impl_->consecutive_speech_frames = 0;
        impl_->consecutive_silence_frames = 0;
        impl_->frame_count = 0;
    }
}

std::string RealSileroVADAdapter::engine_name() const {
    return impl_ && impl_->vad ? "Silero-VAD (Sherpa-ONNX Native)" : "Silero-VAD (Fallback)";
}

const audio::vad::VADConfig& RealSileroVADAdapter::config() const {
    return config_;
}

void RealSileroVADAdapter::set_config(const audio::vad::VADConfig& config) {
    config_ = config;
}

} // namespace vani::adapters::vad
