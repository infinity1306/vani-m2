#include <iostream>
#include <cassert>
#include "../../audio/audio_format.hpp"
#include "../../audio/ring_buffer.hpp"
#include "../../audio/preprocessing/audio_preprocessor.hpp"
#include "../../adapters/vad/silero_vad_adapter.hpp"
#include "../../adapters/wakeword/open_wakeword_adapter.hpp"

using namespace vani::audio;
using namespace vani::audio::preprocessing;
using namespace vani::adapters::vad;
using namespace vani::adapters::wakeword;

void test_ring_buffer() {
    AudioRingBuffer<float> buffer(100);
    assert(buffer.capacity() == 100);
    assert(buffer.empty());

    std::vector<float> input = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    size_t written = buffer.write(input);
    assert(written == 5);
    assert(buffer.size() == 5);

    std::vector<float> output(3);
    size_t read = buffer.read(output);
    assert(read == 3);
    assert(output[0] == 0.1f);
    assert(output[1] == 0.2f);
    assert(output[2] == 0.3f);
    assert(buffer.size() == 2);

    buffer.clear();
    assert(buffer.empty());
    std::cout << "[PASS] AudioRingBuffer test passed.\n";
}

void test_preprocessor() {
    AudioPreprocessor preprocessor;
    assert(preprocessor.stage_count() == 2);

    AudioFormat fmt;
    std::vector<float> noisy_input = {0.005f, 0.008f, 0.3f, -0.4f, 0.002f};
    auto cleaned = preprocessor.process_chain(noisy_input, fmt);
    assert(cleaned.size() == noisy_input.size());
    assert(cleaned[0] == 0.0f); // Gated by noise suppression
    assert(cleaned[1] == 0.0f);

    std::cout << "[PASS] AudioPreprocessor test passed.\n";
}

void test_vad_and_wakeword() {
    SileroVADAdapter vad;
    std::vector<float> silence(160, 0.001f);
    auto vad_res1 = vad.process(silence);
    assert(vad_res1.state == vad::VADState::Silence);

    std::vector<float> speech(160, 0.25f);
    auto vad_res2 = vad.process(speech);
    assert(vad_res2.state == vad::VADState::SpeechStarted);
    assert(vad_res2.is_speech);

    OpenWakeWordAdapter wakeword;
    assert(wakeword.is_enabled());
    wakeword.trigger_manual_wake("vani", 0.95f);
    auto ww_res = wakeword.process(speech);
    assert(ww_res.current_state == wakeword::WakeWordState::WakeDetected);

    std::cout << "[PASS] VAD and WakeWord test passed.\n";
}

int main() {
    std::cout << "--- Running Phase 3 Audio & VAD Unit Tests ---\n";
    test_ring_buffer();
    test_preprocessor();
    test_vad_and_wakeword();
    std::cout << "All Phase 3 Audio unit tests PASSED successfully!\n";
    return 0;
}
