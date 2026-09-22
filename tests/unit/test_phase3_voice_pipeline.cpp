#include <iostream>
#include <cassert>
#include "../../voice/language/language_detector.hpp"
#include "../../voice/normalization/language_normalizer.hpp"
#include "../../voice/vocabulary/vocabulary_engine.hpp"
#include "../../voice/entities/entity_resolver.hpp"
#include "../../voice/intent/intent_preparer.hpp"
#include "../../voice/session/voice_session_manager.hpp"
#include "../../voice/stt/stt_manager.hpp"
#include "../../voice/tts/tts_manager.hpp"
#include "../../adapters/stt/sherpa_onnx_adapter.hpp"
#include "../../adapters/stt/mock_stt_engine.hpp"
#include "../../adapters/tts/mock_tts_engine.hpp"

using namespace vani::voice;
using namespace vani::adapters::stt;
using namespace vani::adapters::tts;

void test_language_detection() {
    auto res_en = language::LanguageDetector::detect("Open Visual Studio Code and build project");
    assert(res_en.iso_code == "en");

    auto res_hinglish = language::LanguageDetector::detect("mera React wala project run karde bhai");
    assert(res_hinglish.iso_code == "hinglish");
    assert(res_hinglish.is_hinglish);

    std::cout << "[PASS] LanguageDetector test passed.\n";
}

void test_normalization_and_entities() {
    normalization::LanguageNormalizer normalizer;
    vani::contracts::STTTranscript raw_t{
        .text = "mera odysus wala react js project rum karde",
        .confidence = 0.95f
    };

    normalization::NormalizationContext ctx{
        .active_app = "VS Code",
        .contextual_terms = {"React", "Odysseus"}
    };

    auto norm_res = normalizer.normalize(raw_t, ctx);
    assert(norm_res.is_ok());
    assert(norm_res.value().normalized_text == "mera Odysseus wala React.js project run kar de");
    assert(norm_res.value().raw_text == "mera odysus wala react js project rum karde");

    auto vocab = std::make_shared<vocabulary::VocabularyEngine>();
    entities::EntityResolver resolver(vocab);
    auto entities = resolver.resolve_entities(norm_res.value().normalized_text, "coding");
    assert(!entities.empty());

    intent::IntentPreparer preparer;
    auto utterance = preparer.prepare("sess-1", norm_res.value(), entities);
    assert(utterance.preferred_route == "agent.odysseus");
    assert(utterance.confidence.overall_confidence() > 0.8f);

    std::cout << "[PASS] Normalization, Vocabulary, and Entity Resolution test passed.\n";
}

void test_stt_tts_managers_and_barge_in() {
    // STT Manager with fallback
    stt::STTManager stt_mgr;
    auto sherpa = std::make_shared<SherpaOnnxAdapter>();
    auto mock_stt = std::make_shared<MockSTTEngine>();
    stt_mgr.register_engine(sherpa, true);
    stt_mgr.set_fallback_engine(mock_stt);

    assert(stt_mgr.registered_engine_count() == 1);
    assert(stt_mgr.active_engine_name() == "Sherpa-ONNX (Streaming)");

    // Fail primary and verify failover
    sherpa->set_healthy(false);
    vani::contracts::STTConfig cfg;
    std::string received_text;
    auto start_res = stt_mgr.start_stream(cfg, [&](const vani::contracts::STTTranscript& t) {
        received_text = t.text;
    });
    assert(start_res.is_ok());
    assert(stt_mgr.fallback_switch_count() == 1);
    stt_mgr.stop_stream();

    // TTS Manager
    tts::TTSManager tts_mgr;
    auto mock_tts = std::make_shared<MockTTSEngine>();
    tts_mgr.register_engine(mock_tts, true);
    auto syn_res = tts_mgr.synthesize("Namaste VANI");
    assert(syn_res.is_ok());
    assert(!syn_res.value().empty());

    // Voice Session & Barge-in
    session::VoiceSessionManager session_mgr;
    auto sid = session_mgr.start_session();
    assert(sid.is_ok());
    assert(session_mgr.current_state() == session::VoiceState::Listening);

    session_mgr.transition_to(session::VoiceState::Speaking);
    assert(session_mgr.current_state() == session::VoiceState::Speaking);

    // Barge-in: user talks while VANI speaks
    session_mgr.trigger_barge_in();
    assert(session_mgr.current_state() == session::VoiceState::Listening); // Resumed listening immediately

    session_mgr.end_session();
    assert(session_mgr.current_state() == session::VoiceState::Idle);

    std::cout << "[PASS] STT, TTS Managers and Barge-in test passed.\n";
}

int main() {
    std::cout << "--- Running Phase 3 Voice Pipeline Unit Tests ---\n";
    test_language_detection();
    test_normalization_and_entities();
    test_stt_tts_managers_and_barge_in();
    std::cout << "All Phase 3 Voice Pipeline unit tests PASSED successfully!\n";
    return 0;
}
