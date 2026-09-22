#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>

#include "voice/evaluation/voice_evaluator.hpp"
#include "audio/ring_buffer.hpp"
#include "voice/telemetry/voice_telemetry.hpp"

using namespace vani;

void test_wer_calculation() {
    std::cout << "[Test 1] Word Error Rate (WER) Algorithm..." << std::endl;
    using namespace voice::evaluation;

    // Identical
    assert(calculate_word_error_rate("open chrome", "open chrome") == 0.0);
    assert(calculate_word_error_rate("Open, Chrome!", "open chrome") == 0.0);

    // Substitution (1 of 2 = 0.5)
    assert(std::abs(calculate_word_error_rate("open chrome", "open edge") - 0.5) < 1e-4);

    // Deletion (1 of 2 = 0.5)
    assert(std::abs(calculate_word_error_rate("open chrome", "open") - 0.5) < 1e-4);

    // Insertion
    assert(std::abs(calculate_word_error_rate("open chrome", "please open chrome now") - 1.0) < 1e-4);

    // Empty cases
    assert(calculate_word_error_rate("", "") == 0.0);
    assert(calculate_word_error_rate("open chrome", "") == 1.0);

    std::cout << "  -> PASSED" << std::endl;
}

void test_percentile_calculation() {
    std::cout << "[Test 2] Latency Percentile Calculation (P50 / P95)..." << std::endl;
    using namespace voice::evaluation;

    std::vector<double> latencies = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0};
    double p50 = calculate_percentile(latencies, 50.0);
    double p95 = calculate_percentile(latencies, 95.0);

    assert(std::abs(p50 - 55.0) < 1e-4);
    assert(p95 >= 95.0 && p95 <= 100.0);

    // Edge cases
    assert(calculate_percentile({}, 50.0) == 0.0);
    assert(calculate_percentile({42.0}, 99.0) == 42.0);

    std::cout << "  -> PASSED" << std::endl;
}

void test_failure_classification() {
    std::cout << "[Test 3] Failure Classification and Diagnosis..." << std::endl;
    using namespace voice::evaluation;

    assert(failure_reason_to_string(VoiceFailureReason::None) == "NONE");
    assert(failure_reason_to_string(VoiceFailureReason::SttLanguageFailure) == "STT_LANGUAGE_FAILURE");
    assert(failure_reason_to_string(VoiceFailureReason::MicCaptureFailure) == "MIC_CAPTURE_FAILURE");
    assert(failure_reason_to_string(VoiceFailureReason::VadFailure) == "VAD_FAILURE");
    assert(failure_reason_to_string(VoiceFailureReason::SttTechnicalVocabularyFailure) == "STT_TECHNICAL_VOCABULARY_FAILURE");
    assert(failure_reason_to_string(VoiceFailureReason::NormalizationFailure) == "NORMALIZATION_FAILURE");
    assert(failure_reason_to_string(VoiceFailureReason::IntentFailure) == "INTENT_FAILURE");

    std::cout << "  -> PASSED" << std::endl;
}

void test_ring_buffer_concurrency_and_overflow() {
    std::cout << "[Test 4] Audio Ring Buffer Overflow Tracking..." << std::endl;
    audio::AudioRingBuffer<float> buffer(1000); // 1000 samples

    std::vector<float> big_chunk(1500, 1.0f);
    size_t written = buffer.write(big_chunk);

    assert(written == 1000);
    assert(buffer.overflow_count() == 500);
    assert(buffer.size() == 1000);

    std::vector<float> out(500);
    size_t read_count = buffer.read(out);
    assert(read_count == 500);
    assert(buffer.size() == 500);

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "         VANI MARK 2 — PHASE 5B VOICE METRICS & WER UNIT TESTS                  " << std::endl;
    std::cout << "================================================================================" << std::endl;

    test_wer_calculation();
    test_percentile_calculation();
    test_failure_classification();
    test_ring_buffer_concurrency_and_overflow();

    std::cout << "\n=== ALL PHASE 5B UNIT TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
