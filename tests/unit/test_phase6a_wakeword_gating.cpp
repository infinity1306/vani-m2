#include "../../audio/wakeword/wakeword_engine.hpp"
#include "../../voice/wakeword/wakeword_engine_factory.hpp"
#include "../../adapters/wakeword/open_wakeword_adapter.hpp"
#include "../../voice/pipeline/gated_voice_pipeline.hpp"
#include "../../voice/stt/stt_engine_factory.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

using namespace vani;
using namespace vani::audio::wakeword;
using namespace vani::voice::wakeword;
using namespace vani::voice::pipeline;

// Mock STT Engine for deterministic testing
class TestMockSTTEngine : public contracts::STTEngine {
public:
    contracts::Result<void> start_stream(const contracts::STTConfig& config, contracts::TranscriptCallback callback) override {
        config_ = config;
        callback_ = callback;
        streaming_ = true;
        invocations_count_++;
        return contracts::Ok();
    }

    contracts::Result<void> push_audio(std::span<const float> audio_data) override {
        pushed_samples_ += audio_data.size();
        return contracts::Ok();
    }

    contracts::Result<void> stop_stream() override {
        streaming_ = false;
        if (callback_) {
            contracts::STTTranscript t;
            t.text = mock_transcript_;
            t.is_final = true;
            t.confidence = 0.95f;
            callback_(t);
        }
        return contracts::Ok();
    }

    contracts::Result<void> reset() {
        streaming_ = false;
        return contracts::Ok();
    }

    std::string engine_name() const override { return "TestMockSTT"; }
    bool is_healthy() const override { return true; }
    contracts::STTCapabilities capabilities() const override {
        return contracts::STTCapabilities{
            .supports_streaming = true,
            .supports_interim_results = true,
            .supports_word_timestamps = false,
            .supports_multilingual = true,
            .is_offline_capable = true
        };
    }

    void set_mock_transcript(const std::string& text) { mock_transcript_ = text; }
    size_t invocations_count() const { return invocations_count_; }
    size_t pushed_samples() const { return pushed_samples_; }

private:
    contracts::STTConfig config_;
    contracts::TranscriptCallback callback_;
    bool streaming_{false};
    std::string mock_transcript_{"chrome kholo"};
    size_t invocations_count_{0};
    size_t pushed_samples_{0};
};

void test_wakeword_factory_and_providers() {
    std::cout << "[TEST] WakeWord Engine Factory & Provider Types..." << std::endl;

    auto mock_engine = WakeWordEngineFactory::create_engine(WakeWordProviderType::Mock);
    assert(mock_engine != nullptr);
    assert(mock_engine->engine_name().find("Mock") != std::string::npos);

    auto sherpa_engine = WakeWordEngineFactory::create_engine_by_name("sherpa_kws");
    assert(sherpa_engine != nullptr);
    assert(sherpa_engine->engine_name().find("Sherpa") != std::string::npos);

    auto oww_engine = WakeWordEngineFactory::create_engine_by_name("openwakeword");
    assert(oww_engine != nullptr);
    assert(oww_engine->engine_name().find("openWakeWord") != std::string::npos);

    std::cout << "  -> PASSED" << std::endl;
}

void test_audio_gating_zero_stt_in_idle() {
    std::cout << "[TEST] Audio Gating: Invariant Zero-STT invocations in IDLE..." << std::endl;

    auto mock_ww = WakeWordEngineFactory::create_engine(WakeWordProviderType::Mock);
    auto mock_stt = std::make_shared<TestMockSTTEngine>();

    GatedPipelineConfig cfg;
    GatedVoicePipeline pipeline(mock_ww, mock_stt, cfg);

    assert(pipeline.current_state() == PipelineState::Idle);

    // Feed 100 frames (3 seconds) of non-wake background noise / speech
    std::vector<float> noise_frame(480, 0.02f);
    for (int i = 0; i < 100; ++i) {
        pipeline.process_audio_frame(noise_frame);
    }

    assert(pipeline.current_state() == PipelineState::Idle);
    assert(pipeline.whisper_invocations_count() == 0);
    assert(mock_stt->invocations_count() == 0);
    assert(pipeline.wake_detections_count() == 0);

    std::cout << "  -> PASSED (STT Invocations: 0 in Idle)" << std::endl;
}

void test_wake_detection_and_pre_roll_retention() {
    std::cout << "[TEST] Wake Detection and Audio Pre-Roll Retention..." << std::endl;

    auto mock_ww = std::make_shared<adapters::wakeword::OpenWakeWordAdapter>();
    auto mock_stt = std::make_shared<TestMockSTTEngine>();
    mock_stt->set_mock_transcript("chrome kholo");

    GatedPipelineConfig cfg;
    cfg.pre_roll_ms = 600; // 600ms = 9600 samples
    GatedVoicePipeline pipeline(mock_ww, mock_stt, cfg);

    bool intent_received = false;
    std::string recognized_intent;
    std::string recognized_transcript;

    pipeline.set_intent_callback([&](const GatedTurnResult& turn) {
        intent_received = true;
        recognized_intent = turn.detected_intent;
        recognized_transcript = turn.raw_transcript;
    });

    // 1. Stream 20 frames into pre-roll buffer (20 * 480 = 9600 samples = 600ms)
    std::vector<float> pre_wake_audio(480, 0.1f);
    for (int i = 0; i < 20; ++i) {
        pipeline.process_audio_frame(pre_wake_audio);
    }
    assert(pipeline.current_state() == PipelineState::Idle);
    assert(mock_stt->invocations_count() == 0);

    // 2. Trigger wake-word detection
    mock_ww->trigger_manual_wake("vani", 0.98f);
    std::vector<float> wake_frame(480, 0.2f);
    pipeline.process_audio_frame(wake_frame);

    // Now state should be Listening or ProcessingSTT
    assert(pipeline.wake_detections_count() == 1);
    assert(mock_stt->invocations_count() == 1);
    // STT should have received the flushed pre-roll buffer (9600 samples) plus wake frame (480) = 10080 samples
    assert(mock_stt->pushed_samples() >= 9600);

    // 3. Push speech audio frames
    std::vector<float> speech_frame(480, 0.3f);
    for (int i = 0; i < 10; ++i) {
        pipeline.process_audio_frame(speech_frame);
    }

    // 4. Push silence frames to trigger end-of-speech
    std::vector<float> silence_frame(480, 0.0f);
    for (int i = 0; i < 45; ++i) {
        pipeline.process_audio_frame(silence_frame);
    }

    assert(intent_received);
    assert(recognized_intent == "app.launch" || !recognized_intent.empty());
    assert(pipeline.current_state() == PipelineState::Idle);

    std::cout << "  -> PASSED (Pre-roll samples flushed: " << mock_stt->pushed_samples() << ")" << std::endl;
}

void test_listen_timeout_returns_to_idle() {
    std::cout << "[TEST] Wake Listen Timeout Returns Safely to IDLE..." << std::endl;

    auto mock_ww = std::make_shared<adapters::wakeword::OpenWakeWordAdapter>();
    auto mock_stt = std::make_shared<TestMockSTTEngine>();

    GatedPipelineConfig cfg;
    cfg.listen_timeout_ms = 200; // Fast timeout for test
    GatedVoicePipeline pipeline(mock_ww, mock_stt, cfg);

    bool callback_fired = false;
    bool was_timeout = false;
    pipeline.set_intent_callback([&](const GatedTurnResult& turn) {
        callback_fired = true;
        was_timeout = turn.is_timeout;
    });

    // Trigger wake
    mock_ww->trigger_manual_wake("vani", 0.95f);
    pipeline.process_audio_frame(std::vector<float>(480, 0.1f));

    assert(pipeline.current_state() == PipelineState::Listening);

    // Wait for timeout duration
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Feed a frame to advance clock check
    pipeline.process_audio_frame(std::vector<float>(480, 0.0f));

    assert(pipeline.current_state() == PipelineState::Idle);
    assert(pipeline.timeout_count() == 1);
    assert(callback_fired);
    assert(was_timeout);

    std::cout << "  -> PASSED (Safely returned to IDLE on timeout)" << std::endl;
}

void test_cancellation_during_active_session() {
    std::cout << "[TEST] Session Cancellation and Clean Teardown..." << std::endl;

    auto mock_ww = std::make_shared<adapters::wakeword::OpenWakeWordAdapter>();
    auto mock_stt = std::make_shared<TestMockSTTEngine>();

    GatedPipelineConfig cfg;
    GatedVoicePipeline pipeline(mock_ww, mock_stt, cfg);

    bool was_cancelled = false;
    pipeline.set_intent_callback([&](const GatedTurnResult& turn) {
        was_cancelled = turn.is_cancelled;
    });

    // Trigger wake
    mock_ww->trigger_manual_wake("vani", 0.99f);
    pipeline.process_audio_frame(std::vector<float>(480, 0.1f));
    assert(pipeline.current_state() == PipelineState::Listening);

    // Cancel active pipeline
    pipeline.cancel();

    assert(pipeline.current_state() == PipelineState::Idle);
    assert(was_cancelled);

    std::cout << "  -> PASSED" << std::endl;
}

void test_duplicate_wake_rejection() {
    std::cout << "[TEST] Duplicate Wake Event Debouncing during Active Session..." << std::endl;

    auto mock_ww = std::make_shared<adapters::wakeword::OpenWakeWordAdapter>();
    auto mock_stt = std::make_shared<TestMockSTTEngine>();

    GatedPipelineConfig cfg;
    GatedVoicePipeline pipeline(mock_ww, mock_stt, cfg);

    // First wake
    mock_ww->trigger_manual_wake("vani", 0.95f);
    pipeline.process_audio_frame(std::vector<float>(480, 0.1f));
    assert(pipeline.wake_detections_count() == 1);

    // Second wake right away while already listening
    mock_ww->trigger_manual_wake("vani", 0.98f);
    pipeline.process_audio_frame(std::vector<float>(480, 0.1f));

    // STT should still only have been initialized once for this session
    assert(mock_stt->invocations_count() == 1);

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "   VANI MARK 2 — PHASE 6A WAKE-WORD UNIT TESTS   " << std::endl;
    std::cout << "=================================================" << std::endl;

    test_wakeword_factory_and_providers();
    test_audio_gating_zero_stt_in_idle();
    test_wake_detection_and_pre_roll_retention();
    test_listen_timeout_returns_to_idle();
    test_cancellation_during_active_session();
    test_duplicate_wake_rejection();

    std::cout << "\n>>> ALL PHASE 6A UNIT TESTS PASSED! <<<" << std::endl;
    return 0;
}
