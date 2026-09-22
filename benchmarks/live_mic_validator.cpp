#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include <windows.h>
#include <psapi.h>

#include <sherpa-onnx/c-api/c-api.h>
#include "audio/input/miniaudio_audio_input.hpp"
#include "adapters/vad/real_silero_vad_adapter.hpp"
#include "adapters/stt/real_sherpa_stt_adapter.hpp"
#include "voice/normalization/language_normalizer.hpp"
#include "voice/vocabulary/vocabulary_engine.hpp"
#include "voice/entities/entity_resolver.hpp"
#include "voice/intent/intent_preparer.hpp"
#include "voice/telemetry/voice_telemetry.hpp"
#include "voice/evaluation/voice_evaluator.hpp"

namespace {

double get_process_memory_mb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
    return 0.0;
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "   VANI MARK 2 — PHASE 5B: REAL HUMAN VOICE & HINGLISH STT VALIDATION            " << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "Target: Physical Microphone (WASAPI) -> Silero VAD -> Sherpa STT -> Intent" << std::endl;
    std::cout << "Mode:   VOICE_TEST_MODE = DIRECT_MIC (Wake-word DISABLED)\n" << std::endl;

    // 1. Initialize Pipeline Engines (Warm Provider Pattern)
    std::cout << "[Step 1] Initializing Real Local Voice Pipeline..." << std::endl;
    auto t_init_start = std::chrono::high_resolution_clock::now();

    vani::audio::AudioFormat mic_format{.sample_rate = 16000, .channels = 1, .format = vani::audio::SampleFormat::Float32};
    vani::audio::MiniaudioAudioInput mic(mic_format, 64000);

    vani::adapters::vad::RealSileroVADAdapter vad_adapter;
    vani::adapters::stt::RealSherpaSTTAdapter stt_adapter;
    vani::voice::normalization::LanguageNormalizer normalizer;
    auto vocab_ptr = std::make_shared<vani::voice::vocabulary::VocabularyEngine>();
    auto entity_resolver = std::make_shared<vani::voice::entities::EntityResolver>(vocab_ptr);
    vani::voice::intent::IntentPreparer intent_preparer(entity_resolver);

    auto t_init_end = std::chrono::high_resolution_clock::now();
    double init_ms = std::chrono::duration<double, std::milli>(t_init_end - t_init_start).count();

    std::cout << "  -> Providers loaded in: " << std::fixed << std::setprecision(1) << init_ms << " ms" << std::endl;
    std::cout << "  -> VAD Engine:  " << vad_adapter.engine_name() << std::endl;
    std::cout << "  -> STT Engine:  " << stt_adapter.engine_name() << std::endl;
    std::cout << "  -> Baseline RAM: " << get_process_memory_mb() << " MB\n" << std::endl;

    // 2. Physical Microphone Check
    std::cout << "[Step 2] Probing Physical Microphone Backend..." << std::endl;
    auto mic_res = mic.start();
    bool physical_mic_active = false;

    if (mic_res.is_ok()) {
        physical_mic_active = true;
        std::cout << "  [SUCCESS] Physical Microphone Initialized & Streaming!" << std::endl;
        std::cout << "  -> Device Name: " << mic.device_name() << std::endl;
        std::cout << "  -> Format:      16,000 Hz, 1-channel Mono PCM Float32" << std::endl;
    } else {
        std::cout << "  [STATUS] PHYSICAL_MIC_TEST_BLOCKED: " << mic_res.error().message << std::endl;
    }

    bool interactive_mode = (argc > 1 && std::string(argv[1]) == "--live");

    if (physical_mic_active && interactive_mode) {
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "   INTERACTIVE LIVE PHYSICAL MICROPHONE LISTENING MODE                          " << std::endl;
        std::cout << "   Speak naturally into your microphone (English / Hindi / Hinglish)...         " << std::endl;
        std::cout << "   (Press Enter or Ctrl+C to stop listening)                                    " << std::endl;
        std::cout << "================================================================================\n" << std::endl;

        std::atomic<bool> keep_running{true};
        std::thread input_thread([&]() {
            std::cin.get();
            keep_running = false;
        });

        const size_t frame_size = 320; // 20ms @ 16kHz
        std::vector<float> frame_buffer(frame_size);
        bool in_speech = false;
        uint32_t partial_count = 0;
        vani::voice::telemetry::VoiceTurnTimestamps turn_ts;
        std::string latest_partial;
        std::string final_transcript_text;
        std::chrono::high_resolution_clock::time_point speech_end_time;

        while (keep_running) {
            if (mic.ring_buffer().size() >= frame_size) {
                size_t read_samples = mic.ring_buffer().read(frame_buffer);
                if (read_samples == frame_size) {
                    auto vad_result = vad_adapter.process(frame_buffer);

                    if (vad_result.state == vani::audio::vad::VADState::SpeechStarted && !in_speech) {
                        in_speech = true;
                        partial_count = 0;
                        latest_partial.clear();
                        final_transcript_text.clear();
                        turn_ts = vani::voice::telemetry::VoiceTurnTimestamps{};
                        turn_ts.turn_id = "live_turn";
                        turn_ts.mark_t0();
                        turn_ts.mark_t1();

                        vani::contracts::STTConfig cfg{.sample_rate_hz = 16000, .language_preference = "auto"};
                        stt_adapter.start_stream(cfg, [&](const vani::contracts::STTTranscript& t) {
                            if (!t.is_final) {
                                if (partial_count == 0) turn_ts.mark_t3();
                                partial_count++;
                                latest_partial = t.text;
                                std::cout << "\r  [STREAMING PARTIAL]: " << latest_partial << std::flush;
                            } else {
                                turn_ts.mark_t4();
                                final_transcript_text = t.text;
                            }
                        });
                        std::cout << "\n[VAD] >> Speech Detected (T1)!" << std::endl;
                    }

                    if (in_speech) {
                        stt_adapter.push_audio(frame_buffer);
                    }

                    if (vad_result.state == vani::audio::vad::VADState::SpeechEnded && in_speech) {
                        speech_end_time = std::chrono::high_resolution_clock::now();
                        in_speech = false;
                        stt_adapter.stop_stream();

                        std::cout << "\n[VAD] >> Speech Ended! Finalizing transcript (T4)..." << std::endl;
                        std::string recognized_str = final_transcript_text.empty() ? latest_partial : final_transcript_text;
                        std::cout << "  -> Final Recognized: \"" << recognized_str << "\"" << std::endl;

                        // Downstream Normalization & Intent
                        vani::contracts::STTTranscript transcript_struct{
                            .text = recognized_str,
                            .is_final = true,
                            .confidence = 0.95f,
                            .detected_language = "auto"
                        };
                        auto norm_res = normalizer.normalize(transcript_struct);
                        turn_ts.mark_t5();

                        std::string norm_text = norm_res.is_ok() ? norm_res.value().normalized_text : recognized_str;
                        auto entities = entity_resolver->resolve_entities(norm_text);
                        vani::voice::normalization::NormalizedText norm_struct{.raw_text = recognized_str, .normalized_text = norm_text};
                        auto utterance = intent_preparer.prepare("live_session", norm_struct, entities);
                        turn_ts.mark_t6();

                        std::string top_intent = utterance.candidate_intents.empty() ? "general_command" : utterance.candidate_intents[0].intent_name;

                        std::cout << "  -> Normalized:        \"" << norm_text << "\"" << std::endl;
                        std::cout << "  -> Intent Formulated: \"" << top_intent << "\"" << std::endl;
                        std::cout << turn_ts.format_stage_breakdown() << std::endl;
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        if (input_thread.joinable()) input_thread.join();
        mic.stop();
        return 0;
    }

    // 3. Automated Human Evaluation Corpus Testing
    std::cout << "[Step 3] Executing Controlled Human Speech & Hinglish Evaluation Suite..." << std::endl;
    std::cout << "Testing 5 Categories with Repeated Measurements (WER, Latency, Accuracy, Failure Classification)\n" << std::endl;

    struct EvaluationCase {
        std::string test_id;
        std::string category;
        std::string wav_file;
        std::string reference_phrase;
        std::string target_intent;
    };

    std::vector<EvaluationCase> evaluation_suite = {
        // A. English
        {"EN-1", "English", "benchmarks/corpus/en_open_chrome.wav", "open chrome", "application.launch"},
        {"EN-2", "English", "benchmarks/corpus/en_open_chrome_fast.wav", "open chrome", "application.launch"},
        {"EN-3", "English", "benchmarks/corpus/en_open_chrome_slow.wav", "open chrome", "application.launch"},
        {"EN-4", "English", "benchmarks/corpus/tech_react_project.wav", "run my react project", "software.execute_workflow"},
        {"EN-5", "English", "benchmarks/corpus/tech_react_project_slow.wav", "run my react project", "software.execute_workflow"},

        // B. Hindi
        {"HI-1", "Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "chrome kholo", "application.launch"},
        {"HI-2", "Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "chrome kholo", "application.launch"},
        {"HI-3", "Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "chrome kholo", "application.launch"},
        {"HI-4", "Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "chrome kholo", "application.launch"},
        {"HI-5", "Hindi", "benchmarks/corpus/hi_chrome_kholo.wav", "chrome kholo", "application.launch"},

        // C. Hinglish
        {"HIN-1", "Hinglish", "benchmarks/corpus/hinglish_chrome_open.wav", "chrome open kar de", "application.launch"},
        {"HIN-2", "Hinglish", "benchmarks/corpus/hinglish_chrome_open_fast.wav", "chrome open kar de", "application.launch"},
        {"HIN-3", "Hinglish", "benchmarks/corpus/hinglish_chrome_open.wav", "chrome open kar de", "application.launch"},
        {"HIN-4", "Hinglish", "benchmarks/corpus/hinglish_chrome_open_fast.wav", "chrome open kar de", "application.launch"},
        {"HIN-5", "Hinglish", "benchmarks/corpus/hinglish_chrome_open.wav", "chrome open kar de", "application.launch"},

        // D. Technical Vocabulary
        {"TECH-1", "Technical", "benchmarks/corpus/tech_react_project.wav", "run my react project", "software.execute_workflow"},
        {"TECH-2", "Technical", "benchmarks/corpus/tech_react_project_slow.wav", "run my react project", "software.execute_workflow"},
        {"TECH-3", "Technical", "benchmarks/corpus/tech_react_project.wav", "run my react project", "software.execute_workflow"},
        {"TECH-4", "Technical", "benchmarks/corpus/tech_react_project_slow.wav", "run my react project", "software.execute_workflow"},
        {"TECH-5", "Technical", "benchmarks/corpus/tech_react_project.wav", "run my react project", "software.execute_workflow"},

        // E. Long Hinglish Commands
        {"LONG-1", "Long Hinglish", "benchmarks/corpus/long_downloads_react.wav", "downloads folder mein jo latest react project hai usko open karke run kar", "software.execute_workflow"},
        {"LONG-2", "Long Hinglish", "benchmarks/corpus/long_downloads_react.wav", "downloads folder mein jo latest react project hai usko open karke run kar", "software.execute_workflow"},
        {"LONG-3", "Long Hinglish", "benchmarks/corpus/long_downloads_react.wav", "downloads folder mein jo latest react project hai usko open karke run kar", "software.execute_workflow"},
        {"LONG-4", "Long Hinglish", "benchmarks/corpus/long_downloads_react.wav", "downloads folder mein jo latest react project hai usko open karke run kar", "software.execute_workflow"},
        {"LONG-5", "Long Hinglish", "benchmarks/corpus/long_downloads_react.wav", "downloads folder mein jo latest react project hai usko open karke run kar", "software.execute_workflow"}
    };

    std::vector<vani::voice::evaluation::UtteranceMetrics> recorded_metrics;

    for (const auto& test : evaluation_suite) {
        const SherpaOnnxWave* wave = SherpaOnnxReadWave(test.wav_file.c_str());
        if (!wave || !wave->samples || wave->num_samples <= 0) {
            std::cerr << "[-] Error reading WAV: " << test.wav_file << std::endl;
            if (wave) SherpaOnnxFreeWave(wave);
            continue;
        }

        double audio_dur_ms = (static_cast<double>(wave->num_samples) * 1000.0) / static_cast<double>(wave->sample_rate);

        vani::voice::telemetry::VoiceTurnTimestamps timestamps;
        timestamps.turn_id = test.test_id;
        timestamps.mark_t0();

        vad_adapter.reset();
        bool speech_started_detected = false;
        uint32_t partial_count = 0;
        std::string latest_partial;
        std::string final_transcript_text;
        std::chrono::high_resolution_clock::time_point speech_start_time;
        std::chrono::high_resolution_clock::time_point speech_end_time;

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

        // Feed in 20ms frames
        const int32_t frame_size = 320;
        for (int32_t offset = 0; offset < wave->num_samples; offset += frame_size) {
            int32_t chunk_len = std::min(frame_size, wave->num_samples - offset);
            std::span<const float> frame(wave->samples + offset, chunk_len);

            auto vad_res = vad_adapter.process(frame);
            if (vad_res.state == vani::audio::vad::VADState::SpeechStarted && !speech_started_detected) {
                speech_started_detected = true;
                speech_start_time = std::chrono::high_resolution_clock::now();
                timestamps.mark_t1();
            }

            stt_adapter.push_audio(frame);
        }

        speech_end_time = std::chrono::high_resolution_clock::now();
        stt_adapter.stop_stream();

        // Downstream Normalization & Intent
        std::string raw_str = final_transcript_text.empty() ? latest_partial : final_transcript_text;
        vani::contracts::STTTranscript transcript_struct{
            .text = raw_str,
            .is_final = true,
            .confidence = 0.95f,
            .detected_language = "auto"
        };
        auto norm_res_obj = normalizer.normalize(transcript_struct);
        timestamps.mark_t5();

        std::string norm_text = norm_res_obj.is_ok() ? norm_res_obj.value().normalized_text : raw_str;
        auto entities = entity_resolver->resolve_entities(norm_text);
        vani::voice::normalization::NormalizedText norm_struct{.raw_text = raw_str, .normalized_text = norm_text};
        auto utterance = intent_preparer.prepare("session_bench", norm_struct, entities);
        timestamps.mark_t6();

        std::string top_intent = utterance.candidate_intents.empty() ? "system.general_command" : utterance.candidate_intents[0].intent_name;

        // WER and Accuracy
        double wer = vani::voice::evaluation::calculate_word_error_rate(test.reference_phrase, norm_text);
        bool accurate = (wer < 0.40) || (top_intent == test.target_intent);

        vani::voice::evaluation::VoiceFailureReason failure = vani::voice::evaluation::VoiceFailureReason::None;
        if (!accurate) {
            if (test.category == "Hindi" || test.category == "Hinglish" || test.category == "Long Hinglish") {
                failure = vani::voice::evaluation::VoiceFailureReason::SttLanguageFailure;
            } else if (test.category == "Technical") {
                failure = vani::voice::evaluation::VoiceFailureReason::SttTechnicalVocabularyFailure;
            } else {
                failure = vani::voice::evaluation::VoiceFailureReason::SttFailure;
            }
        }

        double t0_to_t3 = timestamps.audio_to_first_partial_ms().value_or(0.0);
        double t0_to_t4 = timestamps.audio_to_final_transcript_ms().value_or(0.0);
        double t1_to_t3 = timestamps.t3_first_stt_partial_ns && timestamps.t1_vad_speech_start_ns
            ? static_cast<double>(*timestamps.t3_first_stt_partial_ns - *timestamps.t1_vad_speech_start_ns) / 1e6 : 0.0;
        double t1_to_t4 = timestamps.t4_final_stt_result_ns && timestamps.t1_vad_speech_start_ns
            ? static_cast<double>(*timestamps.t4_final_stt_result_ns - *timestamps.t1_vad_speech_start_ns) / 1e6 : 0.0;
        double speech_end_to_t4 = timestamps.t4_final_stt_result_ns
            ? static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - speech_end_time).count()) / 1e6 : 0.0;
        double total_e2e = timestamps.complete_end_to_end_ms().value_or(0.0);
        double rtf = (t0_to_t4 > 0.0 && audio_dur_ms > 0.0) ? (t0_to_t4 / audio_dur_ms) : 0.0;

        vani::voice::evaluation::UtteranceMetrics m{
            .test_id = test.test_id,
            .category = test.category,
            .reference_phrase = test.reference_phrase,
            .actual_transcript = raw_str,
            .normalized_text = norm_text,
            .detected_intent = top_intent,
            .target_intent = test.target_intent,
            .is_command_accurate = accurate,
            .wer = wer,
            .failure_reason = failure,
            .audio_duration_ms = audio_dur_ms,
            .t0_to_t3_ms = t0_to_t3,
            .t0_to_t4_ms = t0_to_t4,
            .t1_to_t3_ms = t1_to_t3,
            .t1_to_t4_ms = t1_to_t4,
            .speech_end_to_t4_ms = speech_end_to_t4,
            .total_e2e_ms = total_e2e,
            .rtf = rtf,
            .memory_mb = get_process_memory_mb()
        };
        recorded_metrics.push_back(m);
        SherpaOnnxFreeWave(wave);
    }

    // 4. Print Comprehensive Category-Wise Evaluation Report
    std::vector<std::string> categories = {"English", "Hindi", "Hinglish", "Technical", "Long Hinglish"};
    std::cout << "\n========================================================================================================================" << std::endl;
    std::cout << "                 PHASE 5B: HUMAN SPEECH & HINGLISH STT QUALITY MATRIX                                  " << std::endl;
    std::cout << "========================================================================================================================" << std::endl;
    std::cout << "| Category      | Samples | Accurate | Accuracy (%) | Avg WER | P50 T0->T3 | P95 T0->T3 | P50 T0->T4 | P95 T0->T4 | Avg RTF |" << std::endl;
    std::cout << "|---------------|--------:|---------:|-------------:|--------:|-----------:|-----------:|-----------:|-----------:|--------:|" << std::endl;

    for (const auto& cat : categories) {
        size_t total = 0;
        size_t acc = 0;
        double sum_wer = 0.0;
        double sum_rtf = 0.0;
        std::vector<double> t3_vals;
        std::vector<double> t4_vals;

        for (const auto& m : recorded_metrics) {
            if (m.category == cat) {
                total++;
                if (m.is_command_accurate) acc++;
                sum_wer += m.wer;
                sum_rtf += m.rtf;
                t3_vals.push_back(m.t0_to_t3_ms);
                t4_vals.push_back(m.t0_to_t4_ms);
            }
        }

        double acc_pct = total > 0 ? (static_cast<double>(acc) / total) * 100.0 : 0.0;
        double avg_wer = total > 0 ? (sum_wer / total) : 0.0;
        double avg_rtf = total > 0 ? (sum_rtf / total) : 0.0;
        double p50_t3 = vani::voice::evaluation::calculate_percentile(t3_vals, 50.0);
        double p95_t3 = vani::voice::evaluation::calculate_percentile(t3_vals, 95.0);
        double p50_t4 = vani::voice::evaluation::calculate_percentile(t4_vals, 50.0);
        double p95_t4 = vani::voice::evaluation::calculate_percentile(t4_vals, 95.0);

        std::cout << "| " << std::left << std::setw(13) << cat << " | "
                  << std::right << std::setw(7) << total << " | "
                  << std::setw(8) << acc << " | "
                  << std::setw(11) << std::fixed << std::setprecision(1) << acc_pct << "% | "
                  << std::setw(7) << std::setprecision(2) << avg_wer << " | "
                  << std::setw(9) << std::setprecision(1) << p50_t3 << " ms | "
                  << std::setw(9) << std::setprecision(1) << p95_t3 << " ms | "
                  << std::setw(9) << std::setprecision(1) << p50_t4 << " ms | "
                  << std::setw(9) << std::setprecision(1) << p95_t4 << " ms | "
                  << std::setw(6) << std::setprecision(3) << avg_rtf << "x |" << std::endl;
    }
    std::cout << "========================================================================================================================\n" << std::endl;

    // Failure Breakdown
    std::cout << "Failure Mode Classification:" << std::endl;
    size_t failures = 0;
    for (const auto& m : recorded_metrics) {
        if (!m.is_command_accurate) {
            failures++;
            std::cout << "  [" << m.test_id << " - " << m.category << "] Reason: " 
                      << vani::voice::evaluation::failure_reason_to_string(m.failure_reason)
                      << " | Ref: \"" << m.reference_phrase << "\" | Hyp: \"" << m.normalized_text << "\"" << std::endl;
        }
    }
    if (failures == 0) {
        std::cout << "  -> Zero failures encountered across test suite!" << std::endl;
    }

    mic.stop();
    return 0;
}
