#include "../../audio/input/miniaudio_audio_input.hpp"
#include "../../adapters/wakeword/real_sherpa_kws_adapter.hpp"
#include "../../adapters/vad/real_silero_vad_adapter.hpp"
#include <sherpa-onnx/c-api/c-api.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include <span>
#include <mutex>

using namespace vani;

// Computes RMS energy of audio frame
static float compute_rms(std::span<const float> samples) {
    if (samples.empty()) return 0.0f;
    double sum = 0.0;
    for (float s : samples) {
        sum += static_cast<double>(s * s);
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(samples.size())));
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "     VANI MARK 2 — PHASE 7C.1: LIVE WAKE-WORD FORENSIC DIAGNOSTIC HARNESS       \n";
    std::cout << "================================================================================\n\n";

    int max_duration_sec = 15;
    if (argc > 1) {
        try {
            max_duration_sec = std::stoi(argv[1]);
        } catch (...) {}
    }

    // 1. Inspect Exact Runtime Wake Configuration
    std::cout << "[DIAGNOSTIC 1] Exact Production Wake Detector Configuration:\n";
    adapters::wakeword::RealSherpaKwsAdapter::Options kws_opts;
    std::cout << "  Encoder Path:         " << kws_opts.encoder_path << "\n";
    std::cout << "  Decoder Path:         " << kws_opts.decoder_path << "\n";
    std::cout << "  Joiner Path:          " << kws_opts.joiner_path << "\n";
    std::cout << "  Tokens Path:          " << kws_opts.tokens_path << "\n";
    std::cout << "  Raw Keywords String:  \n";
    {
        std::string kw = kws_opts.keywords;
        size_t pos = 0;
        while ((pos = kw.find('\n')) != std::string::npos) {
            std::cout << "    Line: \"" << kw.substr(0, pos) << "\"\n";
            kw.erase(0, pos + 1);
        }
        if (!kw.empty()) {
            std::cout << "    Line: \"" << kw << "\"\n";
        }
    }
    std::cout << "  Keywords Score:       " << kws_opts.keywords_score << "\n";
    std::cout << "  Keywords Threshold:   " << kws_opts.keywords_threshold << "\n";
    std::cout << "  Sample Rate:          16000 Hz\n";
    std::cout << "  Channels:             1 (Mono Float32)\n\n";

    // 2. Initialize Direct C-API Spotter with Inspection Hooks
    SherpaOnnxKeywordSpotterConfig kws_config;
    std::memset(&kws_config, 0, sizeof(kws_config));
    kws_config.feat_config.sample_rate = 16000;
    kws_config.feat_config.feature_dim = 80;
    kws_config.model_config.transducer.encoder = kws_opts.encoder_path.c_str();
    kws_config.model_config.transducer.decoder = kws_opts.decoder_path.c_str();
    kws_config.model_config.transducer.joiner = kws_opts.joiner_path.c_str();
    kws_config.model_config.tokens = kws_opts.tokens_path.c_str();
    kws_config.model_config.provider = kws_opts.provider.c_str();
    kws_config.model_config.num_threads = kws_opts.num_threads;
    kws_config.keywords_buf = kws_opts.keywords.c_str();
    kws_config.keywords_buf_size = static_cast<int32_t>(kws_opts.keywords.size());
    kws_config.keywords_score = kws_opts.keywords_score;
    kws_config.keywords_threshold = kws_opts.keywords_threshold;
    kws_config.max_active_paths = 4;
    kws_config.num_trailing_blanks = 1;

    const SherpaOnnxKeywordSpotter* spotter = SherpaOnnxCreateKeywordSpotter(&kws_config);
    if (!spotter) {
        std::cerr << "[FATAL] Failed to create SherpaOnnxKeywordSpotter!\n";
        return 1;
    }
    const SherpaOnnxOnlineStream* stream = SherpaOnnxCreateKeywordStream(spotter);
    if (!stream) {
        std::cerr << "[FATAL] Failed to create SherpaOnnxKeywordStream!\n";
        SherpaOnnxDestroyKeywordSpotter(spotter);
        return 1;
    }
    std::cout << "  Sherpa-ONNX KeywordSpotter Instance: CREATED & HEALTHY\n";

    // 3. Initialize Production Silero VAD Adapter
    std::cout << "[DIAGNOSTIC 2] Initializing Silero VAD Engine (models/silero_vad.onnx)...\n";
    adapters::vad::RealSileroVADAdapter vad;
    std::cout << "  VAD Engine:           " << vad.engine_name() << " (Model Loaded: " << (vad.is_model_loaded() ? "YES" : "NO") << ")\n\n";

    // 4. Initialize Production Adapter to compare in parallel
    adapters::wakeword::RealSherpaKwsAdapter prod_adapter(kws_opts);
    std::cout << "  Production Adapter:   " << prod_adapter.engine_name() << " (Healthy: " << (prod_adapter.is_healthy() ? "YES" : "NO") << ")\n\n";

    // 5. Initialize Physical Microphone (WASAPI Capture)
    std::cout << "[DIAGNOSTIC 3] Starting Physical Microphone Capture (WASAPI)...\n";
    audio::AudioFormat mic_fmt{.sample_rate = 16000, .channels = 1, .format = audio::SampleFormat::Float32};
    auto mic = std::make_shared<audio::MiniaudioAudioInput>(mic_fmt, 64000);

    std::atomic<uint64_t> total_samples_received{0};
    std::atomic<uint64_t> total_chunks_fed{0};
    std::atomic<uint64_t> total_decode_calls{0};
    std::atomic<uint64_t> speech_frames_count{0};
    std::atomic<bool> wake_detected_any{false};
    std::string detected_keyword_str;
    std::string last_tokens_seen;
    std::string last_json_seen;

    std::mutex inspect_mutex;

    mic->set_callback([&](std::span<const float> frame, const audio::AudioFrame& meta) {
        (void)meta;
        total_samples_received.fetch_add(frame.size(), std::memory_order_relaxed);
        total_chunks_fed.fetch_add(1, std::memory_order_relaxed);

        // 1. Feed frame to VAD (Silero VAD window is 512 samples)
        auto vad_res = vad.process(frame);
        if (vad_res.is_speech) {
            speech_frames_count.fetch_add(1, std::memory_order_relaxed);
        }

        // 2. Feed frame to Production RealSherpaKwsAdapter
        auto prod_res = prod_adapter.process(frame);
        if (prod_res.detected) {
            wake_detected_any.store(true);
            std::lock_guard<std::mutex> lock(inspect_mutex);
            detected_keyword_str = prod_res.wake_word;
        }

        // 3. Feed frame directly to C-API Stream for deep telemetry inspection
        SherpaOnnxOnlineStreamAcceptWaveform(stream, 16000, frame.data(), static_cast<int32_t>(frame.size()));
        while (SherpaOnnxIsKeywordStreamReady(spotter, stream)) {
            SherpaOnnxDecodeKeywordStream(spotter, stream);
            total_decode_calls.fetch_add(1, std::memory_order_relaxed);
        }

        const SherpaOnnxKeywordResult* r = SherpaOnnxGetKeywordResult(spotter, stream);
        if (r) {
            std::lock_guard<std::mutex> lock(inspect_mutex);
            if (r->keyword && std::strlen(r->keyword) > 0) {
                wake_detected_any.store(true);
                detected_keyword_str = r->keyword;
            }
            if (r->tokens && std::strlen(r->tokens) > 0) {
                last_tokens_seen = r->tokens;
            }
            if (r->json && std::strlen(r->json) > 0) {
                last_json_seen = r->json;
            }
            SherpaOnnxDestroyKeywordResult(r);
        }
    });

    auto start_res = mic->start();
    if (!start_res.is_ok()) {
        std::cerr << "[FATAL] Failed to start microphone: " << start_res.error().message << "\n";
        SherpaOnnxDestroyOnlineStream(stream);
        SherpaOnnxDestroyKeywordSpotter(spotter);
        return 1;
    }

    std::cout << "  Device Name:          " << mic->device_name() << "\n";
    std::cout << "  Streaming State:      ACTIVE (16,000 Hz Float32 Mono)\n";
    std::cout << "================================================================================\n";
    std::cout << "  LIVE DIAGNOSTIC RUNNING FOR " << max_duration_sec << " SECONDS.\n";
    std::cout << "  Please speak into your microphone: --> \"VANI\" <-- (or normal background)\n";
    std::cout << "================================================================================\n\n";

    auto t_start = std::chrono::steady_clock::now();
    float max_observed_rms = 0.0f;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - t_start).count();
        if (elapsed >= max_duration_sec) break;

        // Sample current ring buffer to measure RMS
        std::vector<float> probe_buf(480, 0.0f);
        size_t avail = mic->ring_buffer().size();
        float current_rms = 0.0f;
        if (avail >= 480) {
            mic->ring_buffer().read(std::span<float>(probe_buf.data(), probe_buf.size()));
            current_rms = compute_rms(probe_buf);
            if (current_rms > max_observed_rms) max_observed_rms = current_rms;
        }

        uint64_t samples = total_samples_received.load();
        uint64_t chunks = total_chunks_fed.load();
        uint64_t decodes = total_decode_calls.load();
        uint64_t speech = speech_frames_count.load();
        bool wake = wake_detected_any.load();

        std::string tokens_copy, json_copy, kw_copy;
        {
            std::lock_guard<std::mutex> lock(inspect_mutex);
            tokens_copy = last_tokens_seen;
            json_copy = last_json_seen;
            kw_copy = detected_keyword_str;
        }

        std::cout << "[T+" << std::fixed << std::setprecision(1) << elapsed << "s] "
                  << "Samples: " << std::setw(7) << samples << " | "
                  << "Chunks: " << std::setw(4) << chunks << " | "
                  << "Decodes: " << std::setw(4) << decodes << " | "
                  << "RMS: " << std::fixed << std::setprecision(4) << current_rms << " | "
                  << "SpeechFrames: " << std::setw(3) << speech << " | "
                  << "Wake: " << (wake ? ("YES (\"" + kw_copy + "\")") : "NO");
        if (!tokens_copy.empty()) {
            std::cout << " | Tokens: [" << tokens_copy << "]";
        }
        std::cout << "\n";

        if (wake) {
            std::cout << "\n>>> WAKE DETECTED AT T+" << elapsed << "s! Keyword: \"" << kw_copy << "\" <<<\n";
            std::cout << "    JSON: " << json_copy << "\n\n";
            break;
        }
    }

    mic->stop();
    SherpaOnnxDestroyOnlineStream(stream);
    SherpaOnnxDestroyKeywordSpotter(spotter);

    std::cout << "\n================================================================================\n";
    std::cout << "                      DIAGNOSTIC SUMMARY REPORT                                 \n";
    std::cout << "================================================================================\n";
    std::cout << "Total Samples Captured:       " << total_samples_received.load() << "\n";
    std::cout << "Total Audio Chunks Fed:       " << total_chunks_fed.load() << "\n";
    std::cout << "Total Decoder Invocations:    " << total_decode_calls.load() << "\n";
    std::cout << "Max Audio RMS Observed:       " << max_observed_rms << "\n";
    std::cout << "Total Speech Frames Detected: " << speech_frames_count.load() << "\n";
    std::cout << "Acoustic Wake Triggered:      " << (wake_detected_any.load() ? "YES" : "NO") << "\n";
    std::cout << "Configured Threshold:         " << kws_opts.keywords_threshold << "\n";
    std::cout << "Configured Boost Score:       " << kws_opts.keywords_score << "\n";
    std::cout << "================================================================================\n";

    return 0;
}
