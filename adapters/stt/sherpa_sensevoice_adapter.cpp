#include "sherpa_sensevoice_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>
#include <cstring>
#include <vector>
#include <chrono>
#include <regex>

namespace vani::adapters::stt {

static std::string clean_sensevoice_text(const std::string& raw) {
    // Strip emotion/event/language tags like <|zh|>, <|en|>, <|NEUTRAL|>, <|HAPPY|>, <|Speech|>
    std::regex tag_regex(R"(<\|[^|>]+\|>)");
    std::string cleaned = std::regex_replace(raw, tag_regex, "");
    
    // Trim whitespace
    size_t first = cleaned.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = cleaned.find_last_not_of(" \t\n\r");
    return cleaned.substr(first, (last - first + 1));
}

struct SherpaSenseVoiceAdapter::Impl {
    const SherpaOnnxOfflineRecognizer* recognizer{nullptr};
    SherpaSenseVoiceAdapter::Options options;
    contracts::STTConfig config;
    contracts::TranscriptCallback callback;
    std::vector<float> audio_buffer;
    bool is_streaming{false};
    bool is_healthy{true};
    std::string last_partial_text;
    uint64_t sequence_number{0};
    size_t last_partial_sample_count{0};
    std::chrono::high_resolution_clock::time_point stream_start_time;

    Impl(const SherpaSenseVoiceAdapter::Options& opt) : options(opt) {
        SherpaOnnxOfflineRecognizerConfig rec_config;
        std::memset(&rec_config, 0, sizeof(rec_config));
        rec_config.feat_config.sample_rate = options.sample_rate;
        rec_config.feat_config.feature_dim = options.feature_dim;
        rec_config.model_config.sense_voice.model = options.model_path.c_str();
        rec_config.model_config.sense_voice.language = options.language.c_str();
        rec_config.model_config.sense_voice.use_itn = options.use_itn ? 1 : 0;
        rec_config.model_config.tokens = options.tokens_path.c_str();
        rec_config.model_config.num_threads = options.num_threads;
        rec_config.model_config.provider = options.provider.c_str();
        rec_config.model_config.debug = 0;
        rec_config.decoding_method = "greedy_search";

        recognizer = SherpaOnnxCreateOfflineRecognizer(&rec_config);
        if (!recognizer) {
            is_healthy = false;
        }
    }

    ~Impl() {
        if (recognizer) {
            SherpaOnnxDestroyOfflineRecognizer(recognizer);
            recognizer = nullptr;
        }
    }
};

SherpaSenseVoiceAdapter::SherpaSenseVoiceAdapter()
    : SherpaSenseVoiceAdapter(Options{}) {}

SherpaSenseVoiceAdapter::SherpaSenseVoiceAdapter(const Options& options)
    : impl_(std::make_unique<Impl>(options)) {}

SherpaSenseVoiceAdapter::~SherpaSenseVoiceAdapter() = default;
SherpaSenseVoiceAdapter::SherpaSenseVoiceAdapter(SherpaSenseVoiceAdapter&&) noexcept = default;
SherpaSenseVoiceAdapter& SherpaSenseVoiceAdapter::operator=(SherpaSenseVoiceAdapter&&) noexcept = default;

bool SherpaSenseVoiceAdapter::is_healthy() const {
    return impl_ && impl_->is_healthy && impl_->recognizer != nullptr;
}

bool SherpaSenseVoiceAdapter::is_model_loaded() const {
    return impl_ && impl_->recognizer != nullptr;
}

contracts::Result<void> SherpaSenseVoiceAdapter::start_stream(
    const contracts::STTConfig& config,
    contracts::TranscriptCallback callback
) {
    if (!impl_ || !impl_->recognizer) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "SenseVoice model not loaded");
    }

    impl_->config = config;
    impl_->callback = std::move(callback);
    impl_->audio_buffer.clear();
    impl_->is_streaming = true;
    impl_->last_partial_text.clear();
    impl_->sequence_number = 0;
    impl_->last_partial_sample_count = 0;
    impl_->stream_start_time = std::chrono::high_resolution_clock::now();

    return contracts::Ok();
}

contracts::Result<void> SherpaSenseVoiceAdapter::push_audio(std::span<const float> samples) {
    if (!impl_ || !impl_->is_streaming || samples.empty()) {
        return contracts::Ok();
    }

    impl_->audio_buffer.insert(impl_->audio_buffer.end(), samples.begin(), samples.end());

    // Generate interim partials if enabled and we have accumulated enough new audio (e.g., every 8000 samples = 0.5s)
    if (impl_->config.enable_interim_results && 
        impl_->audio_buffer.size() >= 8000 &&
        (impl_->audio_buffer.size() - impl_->last_partial_sample_count) >= 8000) {
        
        const SherpaOnnxOfflineStream* stream = SherpaOnnxCreateOfflineStream(impl_->recognizer);
        if (stream) {
            SherpaOnnxAcceptWaveformOffline(stream, impl_->options.sample_rate, impl_->audio_buffer.data(), static_cast<int32_t>(impl_->audio_buffer.size()));
            SherpaOnnxDecodeOfflineStream(impl_->recognizer, stream);
            const SherpaOnnxOfflineRecognizerResult* res = SherpaOnnxGetOfflineStreamResult(stream);
            if (res && res->text) {
                std::string cleaned = clean_sensevoice_text(res->text);
                if (!cleaned.empty() && cleaned != impl_->last_partial_text) {
                    impl_->last_partial_text = cleaned;
                    impl_->last_partial_sample_count = impl_->audio_buffer.size();
                    
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - impl_->stream_start_time).count();

                    contracts::STTTranscript partial{
                        .text = cleaned,
                        .is_final = false,
                        .confidence = 0.85f,
                        .detected_language = res->lang ? res->lang : "auto",
                        .start_time_ms = 0,
                        .end_time_ms = static_cast<uint64_t>(elapsed_ms),
                        .sequence_number = ++impl_->sequence_number,
                        .engine_name = engine_name()
                    };

                    if (impl_->callback) {
                        impl_->callback(partial);
                    }
                }
            }
            if (res) SherpaOnnxDestroyOfflineRecognizerResult(res);
            SherpaOnnxDestroyOfflineStream(stream);
        }
    }

    return contracts::Ok();
}

contracts::Result<void> SherpaSenseVoiceAdapter::stop_stream() {
    if (!impl_ || !impl_->is_streaming) {
        return contracts::Ok();
    }

    impl_->is_streaming = false;

    if (!impl_->audio_buffer.empty() && impl_->recognizer) {
        const SherpaOnnxOfflineStream* stream = SherpaOnnxCreateOfflineStream(impl_->recognizer);
        if (stream) {
            SherpaOnnxAcceptWaveformOffline(stream, impl_->options.sample_rate, impl_->audio_buffer.data(), static_cast<int32_t>(impl_->audio_buffer.size()));
            SherpaOnnxDecodeOfflineStream(impl_->recognizer, stream);
            const SherpaOnnxOfflineRecognizerResult* res = SherpaOnnxGetOfflineStreamResult(stream);
            
            std::string final_text;
            std::string lang = "auto";
            if (res && res->text) {
                final_text = clean_sensevoice_text(res->text);
                if (res->lang) lang = res->lang;
            }

            auto now = std::chrono::high_resolution_clock::now();
            auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - impl_->stream_start_time).count();

            contracts::STTTranscript final_transcript{
                .text = final_text,
                .is_final = true,
                .confidence = 0.95f,
                .detected_language = lang,
                .start_time_ms = 0,
                .end_time_ms = static_cast<uint64_t>(total_ms),
                .sequence_number = ++impl_->sequence_number,
                .engine_name = engine_name()
            };

            if (impl_->callback) {
                impl_->callback(final_transcript);
            }

            if (res) SherpaOnnxDestroyOfflineRecognizerResult(res);
            SherpaOnnxDestroyOfflineStream(stream);
        }
    }

    impl_->audio_buffer.clear();
    return contracts::Ok();
}

std::string SherpaSenseVoiceAdapter::engine_name() const {
    return "Sherpa-SenseVoice (Small-v1)";
}

contracts::STTCapabilities SherpaSenseVoiceAdapter::capabilities() const {
    return {
        .supports_streaming = true,
        .supports_interim_results = true,
        .supports_word_timestamps = false,
        .supports_multilingual = true,
        .is_offline_capable = true,
        .typical_latency_ms = 35,
        .supported_languages = {"en", "hi", "zh", "ja", "ko", "yue"}
    };
}

} // namespace vani::adapters::stt
