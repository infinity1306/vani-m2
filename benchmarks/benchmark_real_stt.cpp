#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <cmath>
#include <windows.h>
#include <psapi.h>

#include <sherpa-onnx/c-api/c-api.h>
#include "adapters/vad/real_silero_vad_adapter.hpp"
#include "adapters/stt/real_sherpa_stt_adapter.hpp"
#include "voice/normalization/language_normalizer.hpp"
#include "voice/vocabulary/vocabulary_engine.hpp"
#include "voice/entities/entity_resolver.hpp"
#include "voice/intent/intent_preparer.hpp"
#include "voice/telemetry/voice_telemetry.hpp"

namespace {

double get_process_memory_mb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
    return 0.0;
}

} // namespace

struct BenchmarkResult {
    std::string test_name;
    std::string phrase;
    std::string transcription;
    std::string normalized_text;
    std::string intent_name;
    double audio_duration_ms{0.0};
    double first_partial_latency_ms{0.0};
    double final_transcript_latency_ms{0.0};
    double pipeline_total_latency_ms{0.0};
    double rtf{0.0};
    double memory_mb{0.0};
    bool speech_detected_by_vad{false};
    uint32_t partial_count{0};
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "   VANI MARK 2 — PHASE 5A: REAL LOCAL VOICE PIPELINE & LATENCY BENCHMARK        " << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "Hardware: Windows x64 | Engine: Sherpa-ONNX Zipformer-20M + Silero VAD\n" << std::endl;

    // 1. Initialize Adapters & Services (Warm Provider Pattern)
    std::cout << "[1/3] Initializing Neural Providers & Normalization Pipeline..." << std::endl;
    auto t_init_start = std::chrono::high_resolution_clock::now();

    vani::adapters::vad::RealSileroVADAdapter vad_adapter;
    vani::adapters::stt::RealSherpaSTTAdapter stt_adapter;
    vani::voice::normalization::LanguageNormalizer normalizer;
    auto vocab_ptr = std::make_shared<vani::voice::vocabulary::VocabularyEngine>();
    auto entity_resolver = std::make_shared<vani::voice::entities::EntityResolver>(vocab_ptr);
    vani::voice::intent::IntentPreparer intent_preparer(entity_resolver);

    auto t_init_end = std::chrono::high_resolution_clock::now();
    double init_ms = std::chrono::duration<double, std::milli>(t_init_end - t_init_start).count();
    std::cout << "      -> Providers loaded & warmed in: " << std::fixed << std::setprecision(2) << init_ms << " ms" << std::endl;
    std::cout << "      -> Baseline RAM: " << get_process_memory_mb() << " MB" << std::endl;
    std::cout << "      -> VAD healthy: " << (vad_adapter.is_model_loaded() ? "YES" : "FALLBACK") << std::endl;
    std::cout << "      -> STT healthy: " << (stt_adapter.is_healthy() ? "YES" : "NO") << "\n" << std::endl;

    // 2. Define Benchmark Corpus
    struct TestCase {
        std::string name;
        std::string wav_file;
        std::string expected_phrase;
        std::string category;
    };

    std::vector<TestCase> corpus = {
        {"English", "benchmarks/corpus/en_open_chrome.wav", "Open Chrome.", "English"},
        {"Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "Chrome kholo.", "Hindi"},
        {"Hinglish", "benchmarks/corpus/hinglish_chrome_open.wav", "Chrome open kar de.", "Hinglish"},
        {"Technical", "benchmarks/corpus/tech_react_project.wav", "Run my React project.", "Technical"},
        {"Long Command", "benchmarks/corpus/long_downloads_react.wav", "Downloads folder mein jo latest React project hai usko open karke run kar.", "Compound"},
        {"English (Fast)", "benchmarks/corpus/en_open_chrome_fast.wav", "Open Chrome.", "Speed Variation"},
        {"English (Slow)", "benchmarks/corpus/en_open_chrome_slow.wav", "Open Chrome.", "Speed Variation"},
        {"Hinglish (Fast)", "benchmarks/corpus/hinglish_chrome_open_fast.wav", "Chrome open kar de.", "Speed Variation"},
        {"Technical (Slow)", "benchmarks/corpus/tech_react_project_slow.wav", "Run my React project.", "Speed Variation"}
    };

    std::cout << "[2/3] Executing Real Pipeline Benchmarks across " << corpus.size() << " test cases...\n" << std::endl;

    std::vector<BenchmarkResult> results;

    for (const auto& test : corpus) {
        const SherpaOnnxWave* wave = SherpaOnnxReadWave(test.wav_file.c_str());
        if (!wave || !wave->samples || wave->num_samples <= 0) {
            std::cerr << "[-] Error reading WAV via Sherpa-ONNX: " << test.wav_file << std::endl;
            if (wave) SherpaOnnxFreeWave(wave);
            continue;
        }

        double audio_dur_ms = (static_cast<double>(wave->num_samples) * 1000.0) / static_cast<double>(wave->sample_rate);

        vani::voice::telemetry::VoiceTurnTimestamps timestamps;
        timestamps.turn_id = test.name;
        timestamps.mark_t0();

        vad_adapter.reset();
        bool speech_started_detected = false;
        uint32_t partial_count = 0;
        std::string latest_partial;
        std::string final_transcript_text;

        // Start STT Stream
        vani::contracts::STTConfig stt_cfg{.sample_rate_hz = static_cast<uint32_t>(wave->sample_rate), .language_preference = "auto"};
        stt_adapter.start_stream(stt_cfg, [&](const vani::contracts::STTTranscript& t) {
            if (!t.is_final) {
                if (partial_count == 0) {
                    timestamps.mark_t3();
                }
                partial_count++;
                latest_partial = t.text;
            } else {
                timestamps.mark_t4();
                final_transcript_text = t.text;
            }
        });

        // Feed in 20ms frames (320 samples @ 16kHz)
        const int32_t frame_size = 320;
        for (int32_t offset = 0; offset < wave->num_samples; offset += frame_size) {
            int32_t chunk_len = std::min(frame_size, wave->num_samples - offset);
            std::span<const float> frame(wave->samples + offset, chunk_len);

            // VAD
            auto vad_res = vad_adapter.process(frame);
            if (vad_res.state == vani::audio::vad::VADState::SpeechStarted && !speech_started_detected) {
                speech_started_detected = true;
                timestamps.mark_t1();
            }

            // STT Push
            stt_adapter.push_audio(frame);
        }

        // Stop stream to get final result
        stt_adapter.stop_stream();

        // Downstream Pipeline Stages (T5 & T6)
        std::string recognized_str = final_transcript_text.empty() ? latest_partial : final_transcript_text;
        vani::contracts::STTTranscript transcript_struct{
            .text = recognized_str,
            .is_final = true,
            .confidence = 0.95f,
            .detected_language = "auto"
        };
        auto norm_res_obj = normalizer.normalize(transcript_struct);
        timestamps.mark_t5();

        std::string norm_text = norm_res_obj.is_ok() ? norm_res_obj.value().normalized_text : recognized_str;
        auto entities = entity_resolver->resolve_entities(norm_text);
        vani::voice::normalization::NormalizedText norm_struct{.raw_text = recognized_str, .normalized_text = norm_text};
        auto utterance = intent_preparer.prepare("session_bench", norm_struct, entities);
        timestamps.mark_t6();

        std::string top_intent = utterance.candidate_intents.empty() ? "general_command" : utterance.candidate_intents[0].intent_name;

        // Calculate Latencies
        double t0_to_t3 = timestamps.audio_to_first_partial_ms().value_or(0.0);
        double t0_to_t4 = timestamps.audio_to_final_transcript_ms().value_or(0.0);
        double t0_to_t6 = timestamps.complete_end_to_end_ms().value_or(0.0);
        double rtf = (t0_to_t4 > 0.0 && audio_dur_ms > 0.0) ? (t0_to_t4 / audio_dur_ms) : 0.0;

        BenchmarkResult res{
            .test_name = test.name,
            .phrase = test.expected_phrase,
            .transcription = recognized_str,
            .normalized_text = norm_text,
            .intent_name = top_intent,
            .audio_duration_ms = audio_dur_ms,
            .first_partial_latency_ms = t0_to_t3,
            .final_transcript_latency_ms = t0_to_t4,
            .pipeline_total_latency_ms = t0_to_t6,
            .rtf = rtf,
            .memory_mb = get_process_memory_mb(),
            .speech_detected_by_vad = speech_started_detected,
            .partial_count = partial_count
        };
        results.push_back(res);

        std::cout << "  [" << test.name << "]" << std::endl;
        std::cout << "    Audio Duration: " << std::fixed << std::setprecision(1) << audio_dur_ms << " ms" << std::endl;
        std::cout << "    VAD Detected:   " << (speech_started_detected ? "YES" : "NO") << std::endl;
        std::cout << "    T3 1st Partial: " << std::setprecision(1) << t0_to_t3 << " ms (Partials: " << partial_count << ")" << std::endl;
        std::cout << "    T4 Final STT:   " << std::setprecision(1) << t0_to_t4 << " ms" << std::endl;
        std::cout << "    T6 Intent:      " << std::setprecision(1) << t0_to_t6 << " ms" << std::endl;
        std::cout << "    RTF:            " << std::setprecision(4) << rtf << "x" << std::endl;
        std::cout << "    Transcription:  \"" << res.transcription << "\"" << std::endl;
        std::cout << "    Normalized:     \"" << res.normalized_text << "\"" << std::endl;
        std::cout << "    Intent Name:    \"" << res.intent_name << "\"" << std::endl;
        std::cout << "    Memory:         " << std::setprecision(1) << res.memory_mb << " MB\n" << std::endl;

        SherpaOnnxFreeWave(wave);
    }

    // 3. Summary Table
    std::cout << "[3/3] EMPIRICAL BENCHMARK SUMMARY TABLE" << std::endl;
    std::cout << "========================================================================================================================\n";
    std::cout << "| Test Case           | Audio (ms) | 1st Partial (T3) | Final STT (T4) | Intent (T6) | RTF    | RAM (MB) | Transcription\n";
    std::cout << "|---------------------|-----------:|-----------------:|---------------:|------------:|-------:|---------:|--------------------------------------\n";

    double sum_t3 = 0.0;
    double sum_t4 = 0.0;
    double sum_rtf = 0.0;
    for (const auto& r : results) {
        std::cout << "| " << std::left << std::setw(19) << r.test_name << " | "
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1) << r.audio_duration_ms << " | "
                  << std::setw(16) << std::setprecision(1) << r.first_partial_latency_ms << " | "
                  << std::setw(14) << std::setprecision(1) << r.final_transcript_latency_ms << " | "
                  << std::setw(11) << std::setprecision(1) << r.pipeline_total_latency_ms << " | "
                  << std::setw(6) << std::setprecision(3) << r.rtf << " | "
                  << std::setw(8) << std::setprecision(1) << r.memory_mb << " | "
                  << std::left << r.transcription.substr(0, 38) << "\n";
        sum_t3 += r.first_partial_latency_ms;
        sum_t4 += r.final_transcript_latency_ms;
        sum_rtf += r.rtf;
    }
    std::cout << "========================================================================================================================\n";
    if (!results.empty()) {
        std::cout << "Averages: First Partial = " << (sum_t3 / results.size()) << " ms | Final STT = "
                  << (sum_t4 / results.size()) << " ms | RTF = " << (sum_rtf / results.size()) << "x\n";
    }

    return 0;
}
