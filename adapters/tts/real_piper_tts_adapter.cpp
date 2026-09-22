#include "real_piper_tts_adapter.hpp"
#include "sherpa-onnx/c-api/c-api.h"
#include "../../observability/logger.hpp"
#include <cstring>
#include <filesystem>

namespace vani::adapters::tts {

RealPiperTTSAdapter::RealPiperTTSAdapter(const PiperTTSModelConfig& config)
    : config_(config) {
    if (!std::filesystem::exists(config_.model_path) || !std::filesystem::exists(config_.tokens_path)) {
        observability::Logger::instance().warn("PiperTTS", "Model or tokens file missing: " + config_.model_path);
        return;
    }

    SherpaOnnxOfflineTtsConfig tts_cfg;
    std::memset(&tts_cfg, 0, sizeof(tts_cfg));

    tts_cfg.model.vits.model = config_.model_path.c_str();
    tts_cfg.model.vits.tokens = config_.tokens_path.c_str();
    if (!config_.data_dir.empty() && std::filesystem::exists(config_.data_dir)) {
        tts_cfg.model.vits.data_dir = config_.data_dir.c_str();
    }
    if (!config_.lexicon_path.empty() && std::filesystem::exists(config_.lexicon_path)) {
        tts_cfg.model.vits.lexicon = config_.lexicon_path.c_str();
    }
    tts_cfg.model.vits.noise_scale = config_.noise_scale;
    tts_cfg.model.vits.noise_scale_w = config_.noise_scale_w;
    tts_cfg.model.vits.length_scale = config_.length_scale;
    tts_cfg.model.num_threads = config_.num_threads;
    tts_cfg.model.provider = "cpu";
    tts_cfg.model.debug = 0;
    tts_cfg.max_num_sentences = 1;

    tts_handle_ = SherpaOnnxCreateOfflineTts(&tts_cfg);
    if (tts_handle_) {
        initialized_ = true;
        int32_t sr = SherpaOnnxOfflineTtsSampleRate(tts_handle_);
        if (sr > 0) {
            config_.sample_rate = static_cast<uint32_t>(sr);
        }
        observability::Logger::instance().info("PiperTTS", "Initialized successfully voice=" + config_.voice_name);
    } else {
        observability::Logger::instance().error("PiperTTS", "Failed to create offline TTS instance");
    }
}

RealPiperTTSAdapter::~RealPiperTTSAdapter() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tts_handle_) {
        SherpaOnnxDestroyOfflineTts(tts_handle_);
        tts_handle_ = nullptr;
    }
    initialized_ = false;
}

RealPiperTTSAdapter::RealPiperTTSAdapter(RealPiperTTSAdapter&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = std::move(other.config_);
    tts_handle_ = other.tts_handle_;
    initialized_ = other.initialized_;
    other.tts_handle_ = nullptr;
    other.initialized_ = false;
}

RealPiperTTSAdapter& RealPiperTTSAdapter::operator=(RealPiperTTSAdapter&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        if (tts_handle_) {
            SherpaOnnxDestroyOfflineTts(tts_handle_);
        }
        config_ = std::move(other.config_);
        tts_handle_ = other.tts_handle_;
        initialized_ = other.initialized_;
        other.tts_handle_ = nullptr;
        other.initialized_ = false;
    }
    return *this;
}

contracts::Result<std::vector<float>> RealPiperTTSAdapter::synthesize(
    const std::string& text,
    const contracts::TTSConfig& config,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        return contracts::Fail(contracts::ErrorCode::Cancelled, "TTS synthesis cancelled before start");
    }
    if (text.empty()) {
        return contracts::Ok(std::vector<float>{});
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || !tts_handle_) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "Piper TTS engine not initialized");
    }

    SherpaOnnxGenerationConfig gen_cfg;
    std::memset(&gen_cfg, 0, sizeof(gen_cfg));
    gen_cfg.sid = 0;
    gen_cfg.speed = config.speed > 0.0f ? config.speed : 1.0f;
    gen_cfg.silence_scale = 0.2f;

    struct SynthesizeContext {
        contracts::CancellationToken token;
        bool was_cancelled{false};
    } ctx{cancellation_token, false};

    auto callback = [](const float* /*samples*/, int32_t /*n*/, float /*progress*/, void* arg) -> int32_t {
        auto* c = static_cast<SynthesizeContext*>(arg);
        if (c && c->token.is_cancelled()) {
            c->was_cancelled = true;
            return 0; // halt inference immediately
        }
        return 1;
    };

    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerateWithConfig(
        tts_handle_,
        text.c_str(),
        &gen_cfg,
        callback,
        &ctx
    );

    if (ctx.was_cancelled || cancellation_token.is_cancelled()) {
        if (audio) {
            SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        }
        return contracts::Fail(contracts::ErrorCode::Cancelled, "TTS synthesis cancelled during execution");
    }

    if (!audio) {
        return contracts::Fail(contracts::ErrorCode::InternalError, "Piper TTS audio generation returned null");
    }

    std::vector<float> samples;
    if (audio->samples && audio->n > 0) {
        samples.assign(audio->samples, audio->samples + audio->n);
    }

    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    return contracts::Ok(std::move(samples));
}

contracts::Result<void> RealPiperTTSAdapter::synthesize_stream(
    const std::string& text,
    const contracts::TTSConfig& config,
    contracts::AudioChunkCallback chunk_cb,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        return contracts::Fail(contracts::ErrorCode::Cancelled, "Streaming TTS cancelled before start");
    }
    if (text.empty()) {
        return contracts::Ok();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || !tts_handle_) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "Piper TTS engine not initialized");
    }

    SherpaOnnxGenerationConfig gen_cfg;
    std::memset(&gen_cfg, 0, sizeof(gen_cfg));
    gen_cfg.sid = 0;
    gen_cfg.speed = config.speed > 0.0f ? config.speed : 1.0f;
    gen_cfg.silence_scale = 0.2f;

    struct StreamContext {
        contracts::AudioChunkCallback cb;
        contracts::CancellationToken token;
        bool was_cancelled{false};
        int32_t chunk_index{0};
    } ctx{chunk_cb, cancellation_token, false, 0};

    auto callback = [](const float* samples, int32_t n, float progress, void* arg) -> int32_t {
        auto* c = static_cast<StreamContext*>(arg);
        if (!c) return 0;
        if (c->token.is_cancelled()) {
            c->was_cancelled = true;
            return 0; // stop generation
        }
        if (c->cb && samples && n > 0) {
            bool is_final = (progress >= 1.0f);
            c->cb(std::span<const float>(samples, n), is_final);
            c->chunk_index++;
        }
        return 1;
    };

    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerateWithConfig(
        tts_handle_,
        text.c_str(),
        &gen_cfg,
        callback,
        &ctx
    );

    if (ctx.was_cancelled || cancellation_token.is_cancelled()) {
        if (audio) {
            SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        }
        return contracts::Fail(contracts::ErrorCode::Cancelled, "Streaming TTS cancelled by barge-in");
    }

    if (audio) {
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    }

    return contracts::Ok();
}

std::string RealPiperTTSAdapter::engine_name() const {
    return "Piper-VITS (" + config_.voice_name + " / " + config_.language + ")";
}

contracts::TTSCapabilities RealPiperTTSAdapter::capabilities() const {
    return {
        .supports_streaming = true,
        .supports_ssml = false,
        .is_offline_capable = true,
        .typical_ttfa_ms = 45,
        .supported_voices = {config_.voice_name}
    };
}

bool RealPiperTTSAdapter::is_healthy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_ && (tts_handle_ != nullptr);
}

} // namespace vani::adapters::tts
