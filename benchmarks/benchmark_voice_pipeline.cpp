#include <iostream>
#include <chrono>
#include <iomanip>
#include "hinglish_test_dataset.hpp"
#include "../voice/normalization/language_normalizer.hpp"
#include "../voice/intent/intent_preparer.hpp"
#include "../voice/telemetry/voice_telemetry.hpp"

using namespace vani;

int main() {
    std::cout << "===============================================================\n";
    std::cout << "        VANI MARK 2 — PHASE 3 VOICE PIPELINE BENCHMARK        \n";
    std::cout << "===============================================================\n\n";

    auto suite = benchmarks::get_hinglish_benchmark_suite();
    voice::normalization::LanguageNormalizer normalizer;
    voice::intent::IntentPreparer preparer;
    voice::telemetry::VoiceTelemetryCollector telemetry;

    size_t passed_cases = 0;

    std::cout << std::left << std::setw(14) << "Test ID"
              << std::setw(14) << "Category"
              << std::setw(14) << "Latency (us)"
              << std::setw(12) << "Accuracy"
              << "Route" << "\n";
    std::cout << "---------------------------------------------------------------\n";

    for (const auto& tc : suite) {
        auto start = std::chrono::high_resolution_clock::now();

        contracts::STTTranscript raw_t{
            .text = tc.raw_spoken_text,
            .confidence = 0.95f
        };

        auto norm_res = normalizer.normalize(raw_t);
        auto utterance = preparer.prepare("bench-session", norm_res.value());

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        bool is_match = (norm_res.value().normalized_text == tc.expected_normalized_text);
        if (is_match) passed_cases++;

        auto now_ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        voice::telemetry::VoiceTurnTimestamps t_rec;
        t_rec.turn_id = tc.id;
        t_rec.t4_final_stt_result_ns = now_ns;
        t_rec.t5_normalization_complete_ns = now_ns + static_cast<uint64_t>(elapsed_us * 500);
        t_rec.t6_intent_detected_ns = now_ns + static_cast<uint64_t>(elapsed_us * 1000);
        telemetry.record_turn(t_rec);

        std::cout << std::left << std::setw(14) << tc.id
                  << std::setw(14) << tc.category
                  << std::setw(14) << elapsed_us
                  << std::setw(12) << (is_match ? "100%" : "FAIL")
                  << utterance.preferred_route << "\n";
    }

    std::cout << "---------------------------------------------------------------\n";
    std::cout << "Benchmark Complete: " << passed_cases << "/" << suite.size()
              << " cases passed (" << (passed_cases * 100 / suite.size()) << "%)\n";
    std::cout << "Average Pipeline Latency: " << telemetry.average_turn_latency_ms() << " ms (Budget < 20ms)\n\n";

    return 0;
}
