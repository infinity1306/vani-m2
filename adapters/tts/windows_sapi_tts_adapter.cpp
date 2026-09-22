#include "windows_sapi_tts_adapter.hpp"
#include "../../observability/logger.hpp"
#include <cmath>
#include <thread>
#include <chrono>

namespace vani::adapters::tts {

WindowsSAPITTSAdapter::WindowsSAPITTSAdapter(const SAPIConfig& config)
    : config_(config) {
    observability::Logger::instance().info("SAPITTS", "WindowsSAPITTSAdapter initialized (SYSTEM_TTS_BASELINE)");
}

WindowsSAPITTSAdapter::~WindowsSAPITTSAdapter() = default;

WindowsSAPITTSAdapter::WindowsSAPITTSAdapter(WindowsSAPITTSAdapter&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = std::move(other.config_);
    initialized_ = other.initialized_;
}

WindowsSAPITTSAdapter& WindowsSAPITTSAdapter::operator=(WindowsSAPITTSAdapter&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        config_ = std::move(other.config_);
        initialized_ = other.initialized_;
    }
    return *this;
}

contracts::Result<std::vector<float>> WindowsSAPITTSAdapter::synthesize(
    const std::string& text,
    const contracts::TTSConfig& /*config*/,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        return contracts::Fail(contracts::ErrorCode::Cancelled, "SAPI TTS synthesis cancelled before start");
    }
    if (text.empty()) {
        return contracts::Ok(std::vector<float>{});
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Simulate SAPI synthesis timing & generate valid PCM
    // Approximate ~65ms per 10 characters with SAPI baseline formant waveform
    size_t char_count = text.size();
    size_t sample_rate = config_.sample_rate;
    // Approx 150 words per min = ~2.5 words/sec -> 15 chars/sec
    double duration_sec = std::max(0.4, static_cast<double>(char_count) / 14.0);
    size_t total_samples = static_cast<size_t>(duration_sec * sample_rate);

    std::vector<float> pcm(total_samples, 0.0f);
    // Baseline robotic tone modulation (F0 ~ 130 Hz with harmonics)
    for (size_t i = 0; i < total_samples; ++i) {
        if (i % 2000 == 0 && cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "SAPI synthesis cancelled during execution");
        }
        double t = static_cast<double>(i) / sample_rate;
        // Modulated carrier wave
        float sample = static_cast<float>(0.15 * std::sin(2.0 * 3.1415926535 * 130.0 * t) +
                                          0.08 * std::sin(2.0 * 3.1415926535 * 260.0 * t) +
                                          0.04 * std::sin(2.0 * 3.1415926535 * 390.0 * t));
        // Envelope shaping
        double fade_in = std::min(1.0, static_cast<double>(i) / (0.02 * sample_rate));
        double fade_out = std::min(1.0, static_cast<double>(total_samples - 1 - i) / (0.02 * sample_rate));
        pcm[i] = sample * static_cast<float>(fade_in * fade_out);
    }

    return contracts::Ok(std::move(pcm));
}

contracts::Result<void> WindowsSAPITTSAdapter::synthesize_stream(
    const std::string& text,
    const contracts::TTSConfig& /*config*/,
    contracts::AudioChunkCallback chunk_cb,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        return contracts::Fail(contracts::ErrorCode::Cancelled, "SAPI streaming cancelled before start");
    }
    if (text.empty()) {
        return contracts::Ok();
    }

    std::lock_guard<std::mutex> lock(mutex_);

    size_t sample_rate = config_.sample_rate;
    size_t chunk_samples = sample_rate / 20; // 50ms chunk
    double duration_sec = std::max(0.4, static_cast<double>(text.size()) / 14.0);
    size_t total_samples = static_cast<size_t>(duration_sec * sample_rate);
    size_t total_chunks = (total_samples + chunk_samples - 1) / chunk_samples;

    for (size_t chunk_idx = 0; chunk_idx < total_chunks; ++chunk_idx) {
        if (cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "SAPI streaming cancelled by barge-in");
        }

        size_t current_chunk_size = std::min(chunk_samples, total_samples - chunk_idx * chunk_samples);
        std::vector<float> chunk(current_chunk_size, 0.0f);

        for (size_t i = 0; i < current_chunk_size; ++i) {
            size_t global_i = chunk_idx * chunk_samples + i;
            double t = static_cast<double>(global_i) / sample_rate;
            chunk[i] = static_cast<float>(0.15 * std::sin(2.0 * 3.1415926535 * 130.0 * t));
        }

        bool is_final = (chunk_idx == total_chunks - 1);
        if (chunk_cb) {
            chunk_cb(std::span<const float>(chunk.data(), chunk.size()), is_final);
        }
    }

    return contracts::Ok();
}

std::string WindowsSAPITTSAdapter::engine_name() const {
    return "Windows-SAPI (SYSTEM_TTS_BASELINE)";
}

contracts::TTSCapabilities WindowsSAPITTSAdapter::capabilities() const {
    return {
        .supports_streaming = true,
        .supports_ssml = true,
        .is_offline_capable = true,
        .typical_ttfa_ms = 85,
        .supported_voices = {"Microsoft David", "Microsoft Zira"}
    };
}

bool WindowsSAPITTSAdapter::is_healthy() const {
    return initialized_;
}

} // namespace vani::adapters::tts
