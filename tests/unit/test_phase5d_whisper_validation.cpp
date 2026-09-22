#include "../../contracts/providers/stt_engine.hpp"
#include "../../voice/stt/stt_engine_factory.hpp"
#include "../../voice/normalization/language_normalizer.hpp"
#include "../../voice/intent/intent_preparer.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace vani;

void test_whisper_lifecycle_and_streaming() {
    std::cout << "[TEST] Whisper STT Engine lifecycle and stream safety..." << std::endl;

    auto engine = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    assert(engine != nullptr);
    assert(engine->is_healthy());
    assert(engine->engine_name().find("Whisper") != std::string::npos);

    contracts::STTConfig cfg{
        .sample_rate_hz = 16000,
        .channels = 1,
        .language_preference = "auto",
        .enable_interim_results = true
    };

    bool got_callback = false;
    auto start_res = engine->start_stream(cfg, [&](const contracts::STTTranscript& t) {
        got_callback = true;
    });
    assert(start_res.is_ok());

    // Push audio in 30ms chunks
    std::vector<float> chunk(480, 0.0f);
    for (int i = 0; i < 20; ++i) {
        auto push_res = engine->push_audio(chunk);
        assert(push_res.is_ok());
    }

    auto stop_res = engine->stop_stream();
    assert(stop_res.is_ok());
    assert(got_callback);

    std::cout << "  -> PASSED" << std::endl;
}

void test_whisper_session_reuse_and_state_isolation() {
    std::cout << "[TEST] Whisper session reset and cross-session isolation..." << std::endl;

    auto engine = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    assert(engine != nullptr);

    contracts::STTConfig cfg{.sample_rate_hz = 16000, .channels = 1};

    // Cycle 1: Hindi command simulation
    engine->start_stream(cfg, [](const contracts::STTTranscript& t) {});
    std::vector<float> audio1(16000, 0.05f);
    engine->push_audio(audio1);
    engine->stop_stream();

    // Cycle 2: English command simulation (verify no state leakage)
    bool cycle2_ok = false;
    engine->start_stream(cfg, [&](const contracts::STTTranscript& t) {
        if (t.is_final) cycle2_ok = true;
    });
    std::vector<float> audio2(16000, 0.02f);
    engine->push_audio(audio2);
    engine->stop_stream();

    assert(cycle2_ok);
    std::cout << "  -> PASSED" << std::endl;
}

void test_semantic_intent_safety_on_uncertain_audio() {
    std::cout << "[TEST] Intent safety and fallback on ambiguous inputs..." << std::endl;

    voice::normalization::LanguageNormalizer normalizer;
    voice::intent::IntentPreparer intent_preparer;

    // Test with malformed or unknown token
    contracts::STTTranscript uncertain_transcript{
        .text = "xyz unknown garbled noise",
        .is_final = true,
        .confidence = 0.40f,
        .detected_language = "unknown"
    };

    auto norm_res = normalizer.normalize(uncertain_transcript);
    assert(norm_res.is_ok());

    auto utt = intent_preparer.prepare("safe_session", norm_res.value());
    assert(!utt.candidate_intents.empty());
    // Uncertain speech must route to general_command fallback rather than executing critical actions
    assert(utt.candidate_intents[0].intent_name == "system.general_command");
    assert(utt.preferred_route == "runtime.router");

    std::cout << "  -> PASSED" << std::endl;
}

void test_multilingual_number_and_code_switching_normalization() {
    std::cout << "[TEST] Normalizer handling of Hinglish number commands..." << std::endl;

    voice::normalization::LanguageNormalizer normalizer;

    contracts::STTTranscript num_transcript{
        .text = "volume forty kar de",
        .is_final = true,
        .confidence = 0.95f,
        .detected_language = "hinglish"
    };

    auto norm_res = normalizer.normalize(num_transcript);
    assert(norm_res.is_ok());
    assert(!norm_res.value().normalized_text.empty());

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << " RUNNING PHASE 5D WHISPER PRODUCTION UNIT TESTS " << std::endl;
    std::cout << "================================================" << std::endl;

    test_whisper_lifecycle_and_streaming();
    test_whisper_session_reuse_and_state_isolation();
    test_semantic_intent_safety_on_uncertain_audio();
    test_multilingual_number_and_code_switching_normalization();

    std::cout << "\nALL 4/4 PHASE 5D TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
