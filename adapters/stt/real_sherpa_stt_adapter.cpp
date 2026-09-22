#include "real_sherpa_stt_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>
#include <cstring>
#include <iostream>
#include <chrono>

namespace vani::adapters::stt {

struct RealSherpaSTTAdapter::Impl {
    const SherpaOnnxOnlineRecognizer* recognizer{nullptr};
    const SherpaOnnxOnlineStream* stream{nullptr};
    RealSherpaSTTAdapter::Options options;
    contracts::STTConfig config;
    contracts::TranscriptCallback callback;
    bool is_streaming{false};
    bool is_healthy{true};
    std::string last_partial_text;
    uint64_t sequence_number{0};
    uint64_t total_samples_pushed{0};
    std::chrono::high_resolution_clock::time_point stream_start_time;

    Impl(const RealSherpaSTTAdapter::Options& opt) : options(opt) {
        SherpaOnnxOnlineRecognizerConfig rec_config;
        std::memset(&rec_config, 0, sizeof(rec_config));
        rec_config.feat_config.sample_rate = options.sample_rate;
        rec_config.feat_config.feature_dim = options.feature_dim;
        rec_config.model_config.transducer.encoder = options.encoder_path.c_str();
        rec_config.model_config.transducer.decoder = options.decoder_path.c_str();
        rec_config.model_config.transducer.joiner = options.joiner_path.c_str();
        rec_config.model_config.tokens = options.tokens_path.c_str();
        rec_config.model_config.num_threads = options.num_threads;
        rec_config.model_config.provider = options.provider.c_str();
        rec_config.model_config.debug = 0;
        rec_config.decoding_method = options.decoding_method.c_str();
        rec_config.max_active_paths = options.max_active_paths;
        rec_config.enable_endpoint = options.enable_endpoint ? 1 : 0;
        rec_config.rule1_min_trailing_silence = 2.4f;
        rec_config.rule2_min_trailing_silence = 1.0f;
        rec_config.rule3_min_utterance_length = 20.0f;

        recognizer = SherpaOnnxCreateOnlineRecognizer(&rec_config);
        if (!recognizer) {
            is_healthy = false;
        }
    }

    ~Impl() {
        if (stream) {
            SherpaOnnxDestroyOnlineStream(stream);
            stream = nullptr;
        }
        if (recognizer) {
            SherpaOnnxDestroyOnlineRecognizer(recognizer);
            recognizer = nullptr;
        }
    }
};

RealSherpaSTTAdapter::RealSherpaSTTAdapter()
    : RealSherpaSTTAdapter(Options{}) {}

RealSherpaSTTAdapter::RealSherpaSTTAdapter(const Options& options)
    : impl_(std::make_unique<Impl>(options)) {}

RealSherpaSTTAdapter::~RealSherpaSTTAdapter() = default;
RealSherpaSTTAdapter::RealSherpaSTTAdapter(RealSherpaSTTAdapter&&) noexcept = default;
RealSherpaSTTAdapter& RealSherpaSTTAdapter::operator=(RealSherpaSTTAdapter&&) noexcept = default;

bool RealSherpaSTTAdapter::is_healthy() const {
    return impl_ && impl_->is_healthy && impl_->recognizer != nullptr;
}

bool RealSherpaSTTAdapter::is_model_loaded() const {
    return impl_ && impl_->recognizer != nullptr;
}

contracts::Result<void> RealSherpaSTTAdapter::start_stream(
    const contracts::STTConfig& config,
    contracts::TranscriptCallback callback
) {
    if (!impl_ || !impl_->recognizer) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "Sherpa-ONNX model not loaded");
    }

    if (impl_->stream) {
        SherpaOnnxDestroyOnlineStream(impl_->stream);
        impl_->stream = nullptr;
    }

    impl_->config = config;
    impl_->callback = std::move(callback);
    impl_->is_streaming = true;
    impl_->last_partial_text.clear();
    impl_->sequence_number = 0;
    impl_->total_samples_pushed = 0;
    impl_->stream_start_time = std::chrono::high_resolution_clock::now();

    impl_->stream = SherpaOnnxCreateOnlineStream(impl_->recognizer);
    if (!impl_->stream) {
        impl_->is_streaming = false;
        return contracts::Fail(contracts::ErrorCode::InternalError, "Failed to create Sherpa-ONNX online stream");
    }

    return contracts::Ok();
}

contracts::Result<void> RealSherpaSTTAdapter::push_audio(std::span<const float> samples) {
    if (!impl_ || !impl_->is_streaming || !impl_->stream) {
        return contracts::Ok();
    }

    if (samples.empty()) {
        return contracts::Ok();
    }

    impl_->total_samples_pushed += samples.size();

    SherpaOnnxOnlineStreamAcceptWaveform(
        impl_->stream,
        impl_->options.sample_rate,
        samples.data(),
        static_cast<int32_t>(samples.size())
    );

    while (SherpaOnnxIsOnlineStreamReady(impl_->recognizer, impl_->stream)) {
        SherpaOnnxDecodeOnlineStream(impl_->recognizer, impl_->stream);
    }

    const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(impl_->recognizer, impl_->stream);
    if (r) {
        if (r->text && std::strlen(r->text) > 0) {
            std::string current_text(r->text);
            if (current_text != impl_->last_partial_text && impl_->callback) {
                impl_->last_partial_text = current_text;
                impl_->sequence_number++;

                uint64_t duration_ms = (impl_->total_samples_pushed * 1000) / impl_->options.sample_rate;
                contracts::STTTranscript partial{
                    .text = current_text,
                    .is_final = false,
                    .confidence = 0.93f,
                    .detected_language = impl_->config.language_preference.empty() ? "en" : impl_->config.language_preference,
                    .start_time_ms = 0,
                    .end_time_ms = duration_ms,
                    .sequence_number = impl_->sequence_number,
                    .segments = {},
                    .engine_name = engine_name()
                };
                impl_->callback(partial);
            }
        }
        SherpaOnnxDestroyOnlineRecognizerResult(r);
    }

    return contracts::Ok();
}

contracts::Result<void> RealSherpaSTTAdapter::stop_stream() {
    if (!impl_ || !impl_->is_streaming || !impl_->stream) {
        return contracts::Ok();
    }

    SherpaOnnxOnlineStreamInputFinished(impl_->stream);

    while (SherpaOnnxIsOnlineStreamReady(impl_->recognizer, impl_->stream)) {
        SherpaOnnxDecodeOnlineStream(impl_->recognizer, impl_->stream);
    }

    const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(impl_->recognizer, impl_->stream);
    std::string final_text = "";
    if (r) {
        if (r->text) {
            final_text = std::string(r->text);
        }
        SherpaOnnxDestroyOnlineRecognizerResult(r);
    }

    if (impl_->callback) {
        impl_->sequence_number++;
        uint64_t duration_ms = (impl_->total_samples_pushed * 1000) / impl_->options.sample_rate;
        contracts::STTTranscript final_transcript{
            .text = final_text,
            .is_final = true,
            .confidence = final_text.empty() ? 0.0f : 0.95f,
            .detected_language = impl_->config.language_preference.empty() ? "en" : impl_->config.language_preference,
            .start_time_ms = 0,
            .end_time_ms = duration_ms,
            .sequence_number = impl_->sequence_number,
            .segments = {},
            .engine_name = engine_name()
        };
        impl_->callback(final_transcript);
    }

    SherpaOnnxDestroyOnlineStream(impl_->stream);
    impl_->stream = nullptr;
    impl_->is_streaming = false;

    return contracts::Ok();
}

std::string RealSherpaSTTAdapter::engine_name() const {
    return "Sherpa-ONNX Streaming (Zipformer-20M)";
}

contracts::STTCapabilities RealSherpaSTTAdapter::capabilities() const {
    return {
        .supports_streaming = true,
        .supports_interim_results = true,
        .supports_word_timestamps = false,
        .supports_multilingual = true,
        .is_offline_capable = true,
        .typical_latency_ms = 35,
        .supported_languages = {"en", "hi", "hinglish"}
    };
}

} // namespace vani::adapters::stt
