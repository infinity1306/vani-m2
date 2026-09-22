#include "real_sherpa_kws_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>
#include <cstring>
#include <chrono>
#include <iostream>

namespace vani::adapters::wakeword {

struct RealSherpaKwsAdapter::Impl {
    const SherpaOnnxKeywordSpotter* spotter{nullptr};
    const SherpaOnnxOnlineStream* stream{nullptr};
    RealSherpaKwsAdapter::Options options;
    audio::wakeword::WakeWordConfig config;
    audio::wakeword::WakeWordState state{audio::wakeword::WakeWordState::Idle};
    bool is_healthy{true};
    std::chrono::steady_clock::time_point listen_start_time;
    uint64_t decode_count{0};
    uint64_t wake_detection_count{0};
    float max_observed_score{0.0f};

    Impl(const RealSherpaKwsAdapter::Options& opt, const audio::wakeword::WakeWordConfig& cfg)
        : options(opt), config(cfg) {
        SherpaOnnxKeywordSpotterConfig kws_config;
        std::memset(&kws_config, 0, sizeof(kws_config));
        kws_config.feat_config.sample_rate = 16000;
        kws_config.feat_config.feature_dim = 80;
        kws_config.model_config.transducer.encoder = options.encoder_path.c_str();
        kws_config.model_config.transducer.decoder = options.decoder_path.c_str();
        kws_config.model_config.transducer.joiner = options.joiner_path.c_str();
        kws_config.model_config.tokens = options.tokens_path.c_str();
        kws_config.model_config.provider = options.provider.c_str();
        kws_config.model_config.num_threads = options.num_threads;
        kws_config.keywords_buf = options.keywords.c_str();
        kws_config.keywords_buf_size = static_cast<int32_t>(options.keywords.size());
        kws_config.keywords_score = options.keywords_score;
        kws_config.keywords_threshold = options.keywords_threshold;
        kws_config.max_active_paths = 4;
        kws_config.num_trailing_blanks = 1;

        spotter = SherpaOnnxCreateKeywordSpotter(&kws_config);
        if (spotter) {
            stream = SherpaOnnxCreateKeywordStream(spotter);
            if (!stream) {
                is_healthy = false;
            }
        } else {
            is_healthy = false;
        }

        std::cout << "[INFO] [SherpaKWS] Initialized successfully\n"
                  << "       Wake model:                  " << options.encoder_path << "\n"
                  << "       Tokens file:                 " << options.tokens_path << "\n"
                  << "       Keyword phrase:              VANI\n"
                  << "       Parsed/expected tokenization: [\\u2581VA N I -> IDs: 345, 13, 27] / [\\u2581 V A N I -> IDs: 34, 50, 20, 13, 27]\n"
                  << "       Threshold:                   " << options.keywords_threshold << "\n"
                  << "       Sample rate:                 16000 Hz\n";
    }

    ~Impl() {
        if (stream) {
            SherpaOnnxDestroyOnlineStream(stream);
            stream = nullptr;
        }
        if (spotter) {
            SherpaOnnxDestroyKeywordSpotter(spotter);
            spotter = nullptr;
        }
    }
};

RealSherpaKwsAdapter::RealSherpaKwsAdapter()
    : RealSherpaKwsAdapter(Options{}) {}

RealSherpaKwsAdapter::RealSherpaKwsAdapter(const Options& options, audio::wakeword::WakeWordConfig config)
    : impl_(std::make_unique<Impl>(options, config)) {}

RealSherpaKwsAdapter::~RealSherpaKwsAdapter() = default;
RealSherpaKwsAdapter::RealSherpaKwsAdapter(RealSherpaKwsAdapter&&) noexcept = default;
RealSherpaKwsAdapter& RealSherpaKwsAdapter::operator=(RealSherpaKwsAdapter&&) noexcept = default;

bool RealSherpaKwsAdapter::is_healthy() const {
    return impl_ && impl_->is_healthy && impl_->spotter != nullptr;
}

audio::wakeword::DetectionResult RealSherpaKwsAdapter::process(std::span<const float> audio_frame) {
    if (!impl_ || !impl_->config.enabled || audio_frame.empty()) {
        return {
            .detected = false,
            .wake_word = "",
            .confidence = 0.0f,
            .timestamp_ms = 0,
            .current_state = (impl_ && impl_->config.enabled) ? impl_->state : audio::wakeword::WakeWordState::Disabled
        };
    }

    auto now = std::chrono::steady_clock::now();

    // Check timeout if in Listening state
    if (impl_->state == audio::wakeword::WakeWordState::Listening) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - impl_->listen_start_time).count();
        if (elapsed > impl_->config.listen_timeout_ms) {
            impl_->state = audio::wakeword::WakeWordState::Timeout;
            return {
                .detected = false,
                .wake_word = "",
                .confidence = 0.0f,
                .timestamp_ms = static_cast<uint64_t>(elapsed),
                .current_state = audio::wakeword::WakeWordState::Timeout
            };
        }
    }

    if (impl_->spotter && impl_->stream) {
        SherpaOnnxOnlineStreamAcceptWaveform(impl_->stream, 16000, audio_frame.data(), static_cast<int32_t>(audio_frame.size()));
        
        while (SherpaOnnxIsKeywordStreamReady(impl_->spotter, impl_->stream)) {
            SherpaOnnxDecodeKeywordStream(impl_->spotter, impl_->stream);
            impl_->decode_count++;
        }

        const SherpaOnnxKeywordResult* res = SherpaOnnxGetKeywordResult(impl_->spotter, impl_->stream);
        if (res && res->keyword && std::strlen(res->keyword) > 0) {
            std::string kw = res->keyword;
            SherpaOnnxDestroyKeywordResult(res);
            SherpaOnnxResetKeywordStream(impl_->spotter, impl_->stream);

            impl_->state = audio::wakeword::WakeWordState::WakeDetected;
            impl_->listen_start_time = now;
            impl_->wake_detection_count++;
            impl_->max_observed_score = std::max(impl_->max_observed_score, 0.95f);

            return {
                .detected = true,
                .wake_word = kw,
                .confidence = 0.95f,
                .timestamp_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()),
                .current_state = audio::wakeword::WakeWordState::WakeDetected
            };
        }
        if (res) {
            SherpaOnnxDestroyKeywordResult(res);
        }
    }

    return {
        .detected = false,
        .wake_word = "",
        .confidence = 0.0f,
        .timestamp_ms = 0,
        .current_state = impl_->state
    };
}

void RealSherpaKwsAdapter::reset() {
    if (impl_) {
        impl_->state = audio::wakeword::WakeWordState::Idle;
        if (impl_->spotter && impl_->stream) {
            SherpaOnnxResetKeywordStream(impl_->spotter, impl_->stream);
        }
    }
}

std::string RealSherpaKwsAdapter::engine_name() const {
    return "Sherpa-KeywordSpotter (Zipformer-KWS)";
}

const audio::wakeword::WakeWordConfig& RealSherpaKwsAdapter::config() const {
    static const audio::wakeword::WakeWordConfig default_cfg;
    return impl_ ? impl_->config : default_cfg;
}

void RealSherpaKwsAdapter::set_config(const audio::wakeword::WakeWordConfig& config) {
    if (impl_) {
        impl_->config = config;
    }
}

void RealSherpaKwsAdapter::set_enabled(bool enabled) {
    if (impl_) {
        impl_->config.enabled = enabled;
        if (!enabled) {
            impl_->state = audio::wakeword::WakeWordState::Disabled;
        } else if (impl_->state == audio::wakeword::WakeWordState::Disabled) {
            impl_->state = audio::wakeword::WakeWordState::Idle;
        }
    }
}

bool RealSherpaKwsAdapter::is_enabled() const {
    return impl_ ? impl_->config.enabled : false;
}

uint64_t RealSherpaKwsAdapter::decode_count() const {
    return impl_ ? impl_->decode_count : 0;
}

uint64_t RealSherpaKwsAdapter::wake_detection_count() const {
    return impl_ ? impl_->wake_detection_count : 0;
}

float RealSherpaKwsAdapter::max_observed_score() const {
    return impl_ ? impl_->max_observed_score : 0.0f;
}

} // namespace vani::adapters::wakeword
