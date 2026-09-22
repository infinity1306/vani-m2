#include "../../audio/vad/adaptive_vad.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace vani::audio::vad;

static std::vector<float> make_sine_frame(size_t samples, float amplitude, float freq_hz = 440.0f, float sample_rate = 16000.0f) {
    std::vector<float> buf(samples);
    for (size_t i = 0; i < samples; ++i) {
        buf[i] = amplitude * std::sin(2.0f * 3.14159265f * freq_hz * (static_cast<float>(i) / sample_rate));
    }
    return buf;
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "     TEST: ADAPTIVE VAD DETERMINISTIC DYNAMICS        \n";
    std::cout << "======================================================\n";

    AdaptiveVadConfig cfg;
    cfg.warmup_frames = 5; // Fast warmup for test
    AdaptiveVadDetector vad(cfg);

    // 1. Initial State
    std::cout << "[Test 1] Initial State & Baseline Thresholds... ";
    assert(vad.noise_floor_rms() > 0.0001f);
    assert(vad.adaptive_onset_threshold() >= 0.0010f);
    assert(vad.adaptive_offset_threshold() >= 0.0006f);
    assert(!vad.is_speech_active());
    std::cout << "PASSED\n";

    // 2. Ambient Silence Tracking (100 frames of 0.00028 RMS noise)
    std::cout << "[Test 2] Ambient Silence Tracking & Zero False Triggers... ";
    auto silence_frame = make_sine_frame(480, 0.00028f * 1.414f); // sine peak = rms * sqrt(2)
    for (int i = 0; i < 100; ++i) {
        auto res = vad.process_frame(silence_frame);
        assert(!res.is_speech);
        assert(!vad.is_speech_active());
    }
    assert(vad.speech_frame_count() == 0);
    assert(vad.silence_frame_count() > 90);
    // Noise floor should have tracked towards 0.00028
    assert(std::abs(vad.noise_floor_rms() - 0.00028f) < 0.00008f);
    std::cout << "PASSED (Tracked Noise Floor: " << vad.noise_floor_rms() << ")\n";

    // 3. Speech Onset Detection (Human speech ~0.0035 RMS)
    std::cout << "[Test 3] Speech Onset with Attack Debounce... ";
    auto speech_frame = make_sine_frame(480, 0.0035f * 1.414f);
    // Frame 1
    auto r1 = vad.process_frame(speech_frame);
    assert(!r1.is_speech); // Attack debounce (requires 3 frames)
    // Frame 2
    auto r2 = vad.process_frame(speech_frame);
    assert(!r2.is_speech);
    // Frame 3
    auto r3 = vad.process_frame(speech_frame);
    assert(r3.is_speech);
    assert(r3.speech_onset);
    assert(vad.is_speech_active());
    assert(vad.speech_start_timestamp_ns() > 0);
    std::cout << "PASSED (Onset Detected on Frame 3)\n";

    // 4. Noise Floor Freeze During Speech
    std::cout << "[Test 4] Noise Floor Freeze During Active Speech... ";
    float nf_before_speech = vad.noise_floor_rms();
    for (int i = 0; i < 50; ++i) {
        auto res = vad.process_frame(speech_frame);
        assert(res.is_speech);
        assert(vad.is_speech_active());
    }
    float nf_during_speech = vad.noise_floor_rms();
    // Noise floor must not have inflated
    assert(std::abs(nf_during_speech - nf_before_speech) < 0.00001f);
    std::cout << "PASSED (Noise floor frozen at " << nf_during_speech << ")\n";

    // 5. Hysteresis Holding Across Inter-Syllable Dips (~0.0008 RMS)
    std::cout << "[Test 5] Hysteresis Holding Across Syllable Dip... ";
    auto dip_frame = make_sine_frame(480, 0.00085f * 1.414f); // Below onset (0.0010) but above offset (0.0006)
    for (int i = 0; i < 5; ++i) {
        auto res = vad.process_frame(dip_frame);
        assert(res.is_speech); // Must remain active due to hysteresis
        assert(vad.is_speech_active());
    }
    std::cout << "PASSED\n";

    // 6. Silence Release & Hangover Debouncing
    std::cout << "[Test 6] Silence Hangover & Clean Release... ";
    // Feed silence frames (14 frames of hangover)
    for (int i = 0; i < 14; ++i) {
        auto res = vad.process_frame(silence_frame);
        assert(res.is_speech); // Still in hangover
    }
    // 15th frame: release threshold reached
    auto release_res = vad.process_frame(silence_frame);
    assert(!release_res.is_speech);
    assert(release_res.speech_offset);
    assert(!vad.is_speech_active());
    assert(vad.speech_end_timestamp_ns() > 0);
    std::cout << "PASSED (Release on Frame 15)\n";

    // 7. Telemetry Metrics
    std::cout << "[Test 7] Telemetry Metrics Integrity... ";
    assert(vad.speech_frame_count() > 0);
    assert(vad.silence_frame_count() > 0);
    assert(vad.speech_start_timestamp_ns() <= vad.speech_end_timestamp_ns());
    assert(vad.adaptive_vad_threshold() >= 0.0010f);
    std::cout << "PASSED\n";

    std::cout << "\n======================================================\n";
    std::cout << ">>> ALL ADAPTIVE VAD DYNAMICS TESTS PASSED (100%) <<<\n";
    std::cout << "======================================================\n";
    return 0;
}
