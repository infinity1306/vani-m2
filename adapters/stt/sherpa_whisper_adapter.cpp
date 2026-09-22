#include "sherpa_whisper_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>
#include <cstring>
#include <vector>
#include <chrono>

namespace vani::adapters::stt {

struct SherpaWhisperAdapter::Impl {
    const SherpaOnnxOfflineRecognizer* recognizer{nullptr};
    SherpaWhisperAdapter::Options options;
    contracts::STTConfig config;
    contracts::TranscriptCallback callback;
    std::vector<float> audio_buffer;
    bool is_streaming{false};
    bool is_healthy{true};
    std::string last_partial_text;
    uint64_t sequence_number{0};
    size_t last_partial_sample_count{0};
    std::chrono::high_resolution_clock::time_point stream_start_time;

    Impl(const SherpaWhisperAdapter::Options& opt) : options(opt) {
        SherpaOnnxOfflineRecognizerConfig rec_config;
        std::memset(&rec_config, 0, sizeof(rec_config));
        rec_config.feat_config.sample_rate = options.sample_rate;
        rec_config.feat_config.feature_dim = options.feature_dim;
        rec_config.model_config.whisper.encoder = options.encoder_path.c_str();
        rec_config.model_config.whisper.decoder = options.decoder_path.c_str();
        rec_config.model_config.whisper.language = (options.language == "auto" || options.language.empty()) ? "" : options.language.c_str();
        rec_config.model_config.whisper.task = options.task.c_str();
        rec_config.model_config.whisper.tail_paddings = -1;
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

SherpaWhisperAdapter::SherpaWhisperAdapter()
    : SherpaWhisperAdapter(Options{}) {}

SherpaWhisperAdapter::SherpaWhisperAdapter(const Options& options)
    : impl_(std::make_unique<Impl>(options)) {}

SherpaWhisperAdapter::~SherpaWhisperAdapter() = default;
SherpaWhisperAdapter::SherpaWhisperAdapter(SherpaWhisperAdapter&&) noexcept = default;
SherpaWhisperAdapter& SherpaWhisperAdapter::operator=(SherpaWhisperAdapter&&) noexcept = default;

bool SherpaWhisperAdapter::is_healthy() const {
    return impl_ && impl_->is_healthy && impl_->recognizer != nullptr;
}

bool SherpaWhisperAdapter::is_model_loaded() const {
    return impl_ && impl_->recognizer != nullptr;
}

contracts::Result<void> SherpaWhisperAdapter::start_stream(
    const contracts::STTConfig& config,
    contracts::TranscriptCallback callback
) {
    if (!impl_ || !impl_->recognizer) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "Whisper model not loaded");
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

contracts::Result<void> SherpaWhisperAdapter::push_audio(std::span<const float> samples) {
    if (!impl_ || !impl_->is_streaming || samples.empty()) {
        return contracts::Ok();
    }

    impl_->audio_buffer.insert(impl_->audio_buffer.end(), samples.begin(), samples.end());

    // Generate interim partials if enabled and enough audio is accumulated
    if (impl_->config.enable_interim_results && 
        impl_->audio_buffer.size() >= 16000 &&
        (impl_->audio_buffer.size() - impl_->last_partial_sample_count) >= 16000) {
        
        const SherpaOnnxOfflineStream* stream = SherpaOnnxCreateOfflineStream(impl_->recognizer);
        if (stream) {
            SherpaOnnxAcceptWaveformOffline(stream, impl_->options.sample_rate, impl_->audio_buffer.data(), static_cast<int32_t>(impl_->audio_buffer.size()));
            SherpaOnnxDecodeOfflineStream(impl_->recognizer, stream);
            const SherpaOnnxOfflineRecognizerResult* res = SherpaOnnxGetOfflineStreamResult(stream);
            if (res && res->text && std::strlen(res->text) > 0) {
                std::string txt = res->text;
                if (txt != impl_->last_partial_text) {
                    impl_->last_partial_text = txt;
                    impl_->last_partial_sample_count = impl_->audio_buffer.size();
                    
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - impl_->stream_start_time).count();

                    contracts::STTTranscript partial{
                        .text = txt,
                        .is_final = false,
                        .confidence = 0.90f,
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

contracts::Result<void> SherpaWhisperAdapter::stop_stream() {
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
                final_text = res->text;
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

std::string SherpaWhisperAdapter::engine_name() const {
    return "Sherpa-Whisper (Tiny-Multilingual)";
}

contracts::STTCapabilities SherpaWhisperAdapter::capabilities() const {
    return {
        .supports_streaming = false,
        .supports_interim_results = true,
        .supports_word_timestamps = false,
        .supports_multilingual = true,
        .is_offline_capable = true,
        .typical_latency_ms = 110,
        .supported_languages = {"en", "hi", "es", "fr", "de", "zh", "ja"}
    };
}

} // namespace vani::adapters::stt
