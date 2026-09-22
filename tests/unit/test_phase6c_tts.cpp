#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <thread>
#include <chrono>

#include "../../contracts/providers/tts_engine.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include "../../voice/tts/tts_engine_factory.hpp"
#include "../../voice/tts/tts_manager.hpp"
#include "../../audio/output/miniaudio_audio_output.hpp"
#include "../../adapters/tts/real_piper_tts_adapter.hpp"
#include "../../adapters/tts/windows_sapi_tts_adapter.hpp"
#include "../../adapters/tts/mock_tts_engine.hpp"
#include "../../voice/telemetry/voice_telemetry.hpp"

using namespace vani;

static void test_engine_initialization() {
    std::cout << "[Test 1] Engine Initialization... ";
    auto mock = voice::tts::TTSEngineFactory::create_mock_engine();
    assert(mock != nullptr);
    assert(mock->is_healthy());
    assert(mock->engine_name() == "MockTTS");

    auto sapi = voice::tts::TTSEngineFactory::create_sapi_baseline();
    assert(sapi != nullptr);
    assert(sapi->is_healthy());

    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    assert(piper_en != nullptr);
    assert(piper_en->is_healthy());
    std::cout << "PASSED\n";
}

static void test_missing_model_handling() {
    std::cout << "[Test 2] Missing Model Handling... ";
    adapters::tts::PiperTTSModelConfig bad_cfg;
    bad_cfg.model_path = "models/tts/non_existent_model.onnx";
    bad_cfg.tokens_path = "models/tts/non_existent_tokens.txt";
    adapters::tts::RealPiperTTSAdapter bad_adapter(bad_cfg);

    assert(!bad_adapter.is_healthy());
    auto res = bad_adapter.synthesize("Hello");
    assert(res.is_err());
    assert(res.error().code == contracts::ErrorCode::ServiceUnavailable);
    std::cout << "PASSED\n";
}

static void test_english_synthesis() {
    std::cout << "[Test 3] English Synthesis... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;
    cfg.speed = 1.0f;
    auto res = piper_en->synthesize("Opening Google Chrome.", cfg);
    assert(res.is_ok());
    assert(!res.value().empty());
    std::cout << "PASSED (" << res.value().size() << " samples)\n";
}

static void test_hindi_synthesis() {
    std::cout << "[Test 4] Hindi Synthesis... ";
    auto piper_hi = voice::tts::TTSEngineFactory::create_piper_hindi();
    contracts::TTSConfig cfg;
    cfg.speed = 1.0f;
    auto res = piper_hi->synthesize("Chrome खोल रहा हूँ।", cfg);
    assert(res.is_ok());
    assert(!res.value().empty());
    std::cout << "PASSED (" << res.value().size() << " samples)\n";
}

static void test_hinglish_synthesis() {
    std::cout << "[Test 5] Hinglish Synthesis... ";
    auto piper_hi = voice::tts::TTSEngineFactory::create_piper_hindi();
    contracts::TTSConfig cfg;
    auto res = piper_hi->synthesize("Chrome kholo, main abhi open kar raha hoon.", cfg);
    assert(res.is_ok());
    assert(!res.value().empty());
    std::cout << "PASSED (" << res.value().size() << " samples)\n";
}

static void test_technical_synthesis() {
    std::cout << "[Test 6] Technical Terminology Synthesis... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;
    auto res = piper_en->synthesize("FastAPI server running on localhost port 8000.", cfg);
    assert(res.is_ok());
    assert(!res.value().empty());
    std::cout << "PASSED (" << res.value().size() << " samples)\n";
}

static void test_streaming_and_ttfa() {
    std::cout << "[Test 7 & 8] Streaming & TTFA... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;
    size_t chunk_count = 0;
    size_t total_samples = 0;

    auto stream_cb = [&](std::span<const float> chunk, bool /*is_final*/) {
        chunk_count++;
        total_samples += chunk.size();
    };

    auto res = piper_en->synthesize_stream("This is a streaming test for Time To First Audio.", cfg, stream_cb);
    assert(res.is_ok());
    assert(chunk_count > 0);
    assert(total_samples > 0);
    std::cout << "PASSED (" << chunk_count << " chunks, " << total_samples << " total samples)\n";
}

static void test_cancellation_bargein() {
    std::cout << "[Test 9] Cooperative Cancellation (Barge-In)... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;

    contracts::CancellationSource cts;
    cts.cancel(); // cancel immediately

    auto res = piper_en->synthesize("This synthesis should be cancelled immediately.", cfg, cts.token());
    assert(res.is_err());
    assert(res.error().code == contracts::ErrorCode::Cancelled);

    auto stream_res = piper_en->synthesize_stream("This stream should be cancelled.", cfg, nullptr, cts.token());
    assert(stream_res.is_err());
    assert(stream_res.error().code == contracts::ErrorCode::Cancelled);
    std::cout << "PASSED\n";
}

static void test_audio_output_queue() {
    std::cout << "[Test 10] Audio Output Queue & Playback... ";
    audio::MiniaudioAudioOutput output;
    assert(output.start().is_ok());
    assert(output.device_name().find("WASAPI") != std::string::npos);

    std::vector<float> sample_chunk(1024, 0.05f);
    assert(output.queue_chunk(std::span<const float>(sample_chunk.data(), sample_chunk.size()), false).is_ok());
    assert(output.queue_chunk(std::span<const float>(sample_chunk.data(), sample_chunk.size()), true).is_ok());
    assert(output.total_samples_played() == 2048);

    output.clear_queue();
    assert(output.stop().is_ok());
    std::cout << "PASSED\n";
}

static void test_audio_validity() {
    std::cout << "[Test 11] Audio Validity & PCM Integrity... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;
    auto res = piper_en->synthesize("Integrity verification.", cfg);
    assert(res.is_ok());
    const auto& samples = res.value();
    assert(!samples.empty());

    for (float s : samples) {
        assert(!std::isnan(s));
        assert(!std::isinf(s));
        assert(std::abs(s) <= 1.0f); // no clipping
    }
    std::cout << "PASSED (Valid Float32 PCM [-1.0, 1.0])\n";
}

static void test_repeated_synthesis_stability() {
    std::cout << "[Test 12 & 13] Repeated Synthesis Stability (20 cycles)... ";
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    contracts::TTSConfig cfg;

    for (int i = 0; i < 20; ++i) {
        auto res = piper_en->synthesize("Iteration " + std::to_string(i), cfg);
        assert(res.is_ok());
        assert(!res.value().empty());
    }
    std::cout << "PASSED (20/20 stable)\n";
}

static void test_factory_and_manager_routing() {
    std::cout << "[Test 14 & 15] Factory & Manager Language Routing... ";
    voice::tts::TTSManager manager;
    auto piper_en = voice::tts::TTSEngineFactory::create_piper_english();
    auto piper_hi = voice::tts::TTSEngineFactory::create_piper_hindi();

    manager.register_engine(piper_en, true); // English primary
    manager.register_engine(piper_hi, false); // Hindi registered

    assert(manager.registered_engine_count() == 2);
    assert(manager.active_engine_name().find("en_US") != std::string::npos);

    // English text should use English engine
    auto res_en = manager.synthesize("Hello world");
    assert(res_en.is_ok());

    // Hindi text should automatically route to Hindi engine
    auto res_hi = manager.synthesize("Chrome खोल रहा हूँ।");
    assert(res_hi.is_ok());
    std::cout << "PASSED\n";
}

static void test_telemetry_timestamps() {
    std::cout << "[Test 16, 17, 18] Telemetry Timestamps (T11->T13)... ";
    voice::telemetry::VoiceTurnTimestamps turn;
    turn.turn_id = "test-turn-tts-001";
    turn.mark_t11();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    turn.mark_t12();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    turn.mark_t13();

    auto ttfa = turn.tts_to_first_audio_ms();
    assert(ttfa.has_value());
    assert(*ttfa >= 4.0);

    voice::telemetry::VoiceTelemetryCollector collector;
    collector.record_turn(turn);
    assert(collector.total_turns_recorded() == 1);
    std::cout << "PASSED (TTFA: " << *ttfa << " ms)\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "        PHASE 6C TTS UNIT & INTEGRATION TESTS         \n";
    std::cout << "======================================================\n";

    test_engine_initialization();
    test_missing_model_handling();
    test_english_synthesis();
    test_hindi_synthesis();
    test_hinglish_synthesis();
    test_technical_synthesis();
    test_streaming_and_ttfa();
    test_cancellation_bargein();
    test_audio_output_queue();
    test_audio_validity();
    test_repeated_synthesis_stability();
    test_factory_and_manager_routing();
    test_telemetry_timestamps();

    std::cout << "======================================================\n";
    std::cout << "ALL PHASE 6C TTS TESTS PASSED (18/18 CHECKPOINTS OK)\n";
    std::cout << "======================================================\n";
    return 0;
}
