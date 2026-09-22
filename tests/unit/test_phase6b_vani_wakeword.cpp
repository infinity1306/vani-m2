#include "../../audio/wakeword/wakeword_engine.hpp"
#include "../../audio/wakeword/model_metadata.hpp"
#include "../../adapters/wakeword/dedicated_vani_wakeword_adapter.hpp"
#include "../../voice/wakeword/wakeword_engine_factory.hpp"
#include "../../voice/pipeline/gated_voice_pipeline.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

using namespace vani;
using namespace vani::audio::wakeword;
using namespace vani::adapters::wakeword;
using namespace vani::voice::wakeword;
using namespace vani::voice::pipeline;

// Mock STT Engine for deterministic testing
class Phase6BMockSTTEngine : public contracts::STTEngine {
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
            t.confidence = 0.98f;
            callback_(t);
        }
        return contracts::Ok();
    }

    std::string engine_name() const override { return "Phase6BMockSTT"; }
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

void test_metadata_and_model_versioning() {
    std::cout << "[TEST] Dedicated VANI Wake-Word Metadata & Versioning..." << std::endl;

    DedicatedVaniWakeWordAdapter adapter;
    auto meta = adapter.metadata();

    assert(meta.wake_word == "VANI");
    assert(meta.is_dedicated_model == true);
    assert(meta.sample_rate == 16000);
    assert(!meta.model_name.empty());
    assert(!meta.model_version.empty());
    assert(!meta.model_hash.empty());
    assert(!meta.training_dataset_version.empty());

    std::cout << "  -> Model Name:        " << meta.model_name << std::endl;
    std::cout << "  -> Model Version:     " << meta.model_version << std::endl;
    std::cout << "  -> Target Wake Word:  " << meta.wake_word << std::endl;
    std::cout << "  -> Dataset Version:   " << meta.training_dataset_version << std::endl;
    std::cout << "  -> Architecture Type: " << meta.architecture_type << std::endl;
    std::cout << "  -> PASSED" << std::endl;
}

void test_factory_creation_and_routing() {
    std::cout << "[TEST] WakeWord Engine Factory with Dedicated VANI Provider..." << std::endl;

    auto engine = WakeWordEngineFactory::create_engine(WakeWordProviderType::DedicatedVani);
    assert(engine != nullptr);
    assert(engine->engine_name().find("Dedicated-VANI") != std::string::npos);

    auto engine_by_name = WakeWordEngineFactory::create_engine_by_name("dedicated_vani");
    assert(engine_by_name != nullptr);
    assert(engine_by_name->engine_name().find("Dedicated-VANI") != std::string::npos);

    std::cout << "  -> PASSED" << std::endl;
}

void test_threshold_calibration_and_acoustic_gating() {
    std::cout << "[TEST] Threshold Calibration & Sensitivity Control..." << std::endl;

    DedicatedVaniWakeWordAdapter adapter;
    
    // Low energy noise frame (energy < threshold)
    std::vector<float> noise_frame(480, 0.005f);
    auto res_noise = adapter.process(noise_frame);
    assert(!res_noise.detected);

    // Calibrate threshold
    adapter.calibrate_threshold(0.85f);
    assert(adapter.config().detection_threshold == 0.85f);

    // High energy test frame
    std::vector<float> loud_frame(480, 0.35f);
    auto res_loud = adapter.process(loud_frame);
    assert(res_loud.detected);
    assert(res_loud.wake_word == "VANI");

    std::cout << "  -> PASSED" << std::endl;
}

void test_gated_pipeline_with_dedicated_adapter() {
    std::cout << "[TEST] Gated Voice Pipeline with Dedicated VANI Model..." << std::endl;

    auto dedicated_engine = std::make_shared<DedicatedVaniWakeWordAdapter>();
    auto mock_stt = std::make_shared<Phase6BMockSTTEngine>();
    mock_stt->set_mock_transcript("chrome kholo");

    GatedPipelineConfig pipe_cfg{
        .sample_rate = 16000,
        .pre_roll_ms = 600,
        .listen_timeout_ms = 3500,
        .enable_audio_gating = true
    };

    GatedVoicePipeline pipeline(dedicated_engine, mock_stt, pipe_cfg);

    bool turn_completed = false;
    std::string recognized_intent;
    pipeline.set_intent_callback([&](const GatedTurnResult& turn) {
        turn_completed = true;
        recognized_intent = turn.detected_intent;
    });

    // 1. Idle noise stream -> Zero STT
    std::vector<float> idle_noise(480, 0.001f);
    for (int i = 0; i < 20; ++i) {
        pipeline.process_audio_frame(idle_noise);
    }
    assert(pipeline.current_state() == PipelineState::Idle);
    assert(mock_stt->invocations_count() == 0);

    // 2. Trigger wake detection
    dedicated_engine->trigger_simulated_detection("VANI", 0.98f);
    pipeline.process_audio_frame(std::vector<float>(480, 0.1f));

    assert(pipeline.current_state() == PipelineState::Listening);
    assert(mock_stt->invocations_count() == 1);
    assert(mock_stt->pushed_samples() >= 9600); // 600ms pre-roll flushed

    // 3. Command stream
    std::vector<float> command_audio(480, 0.2f);
    for (int i = 0; i < 10; ++i) {
        pipeline.process_audio_frame(command_audio);
    }

    // 4. Silence to conclude speech turn
    for (int i = 0; i < 45; ++i) {
        pipeline.process_audio_frame(idle_noise);
    }

    assert(turn_completed);
    assert(recognized_intent == "app.launch" || !recognized_intent.empty());
    assert(pipeline.current_state() == PipelineState::Idle);

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   VANI MARK 2 — PHASE 6B DEDICATED WAKE-WORD TESTS    \n";
    std::cout << "========================================================\n";

    test_metadata_and_model_versioning();
    test_factory_creation_and_routing();
    test_threshold_calibration_and_acoustic_gating();
    test_gated_pipeline_with_dedicated_adapter();

    std::cout << "\n>>> ALL PHASE 6B DEDICATED WAKE-WORD TESTS PASSED! <<<\n";
    return 0;
}
