#include <iostream>
#include <cassert>
#include "../../audio/audio_format.hpp"
#include "../../audio/preprocessing/audio_preprocessor.hpp"
#include "../../adapters/vad/silero_vad_adapter.hpp"
#include "../../adapters/wakeword/open_wakeword_adapter.hpp"
#include "../../adapters/stt/mock_stt_engine.hpp"
#include "../../adapters/tts/mock_tts_engine.hpp"
#include "../../voice/normalization/language_normalizer.hpp"
#include "../../voice/vocabulary/vocabulary_engine.hpp"
#include "../../voice/entities/entity_resolver.hpp"
#include "../../voice/intent/intent_preparer.hpp"
#include "../../voice/session/voice_session_manager.hpp"

using namespace vani;

int main() {
    std::cout << "--- Running Phase 3 End-to-End Reference Voice Flow ---\n";

    // 1. Audio Capture Simulation
    audio::AudioFormat format{.sample_rate = 16000, .channels = 1};
    std::vector<float> incoming_pcm(3200, 0.2f); // 200ms speech chunk

    // 2. Preprocessing Chain
    audio::preprocessing::AudioPreprocessor preprocessor;
    auto cleaned_pcm = preprocessor.process_chain(incoming_pcm, format);
    assert(cleaned_pcm.size() == incoming_pcm.size());

    // 3. Voice Activity Detection
    adapters::vad::SileroVADAdapter vad;
    auto vad_res = vad.process(cleaned_pcm);
    assert(vad_res.is_speech);

    // 4. Wake-word Detection
    adapters::wakeword::OpenWakeWordAdapter wakeword;
    wakeword.trigger_manual_wake("vani", 0.96f);

    // 5. Session State Machine Activation
    voice::session::VoiceSessionManager session_mgr;
    auto sid = session_mgr.start_session("mic-default", "speaker-default");
    assert(sid.is_ok());

    // 6. Streaming STT
    adapters::stt::MockSTTEngine stt_engine("odysus ko mera react js project run karne bolo");
    contracts::STTTranscript final_transcript;
    contracts::STTConfig stt_cfg;
    stt_engine.start_stream(stt_cfg, [&](const contracts::STTTranscript& t) {
        if (t.is_final) {
            final_transcript = t;
        }
    });
    stt_engine.push_audio(cleaned_pcm);
    stt_engine.stop_stream();

    assert(final_transcript.is_final);
    assert(final_transcript.text == "odysus ko mera react js project run karne bolo");

    // 7. Normalization & Vocabulary
    voice::normalization::LanguageNormalizer normalizer;
    auto norm_res = normalizer.normalize(final_transcript);
    assert(norm_res.is_ok());
    assert(norm_res.value().normalized_text == "Odysseus ko mera React.js project run karne bolo");
    assert(norm_res.value().raw_text == "odysus ko mera react js project run karne bolo"); // Preserved raw!

    // 8. Entity Resolution
    auto vocab = std::make_shared<voice::vocabulary::VocabularyEngine>();
    voice::entities::EntityResolver resolver(vocab);
    auto entities = resolver.resolve_entities(norm_res.value().normalized_text);
    assert(!entities.empty());

    // 9. Intent Preparation for VANI Runtime
    voice::intent::IntentPreparer intent_prep;
    auto utterance = intent_prep.prepare(sid.value(), norm_res.value(), entities);
    assert(utterance.preferred_route == "agent.odysseus");
    assert(utterance.confidence.overall_confidence() > 0.85f);

    // 10. Synthesize Voice Response & Barge-in Test
    session_mgr.transition_to(voice::session::VoiceState::Speaking);
    adapters::tts::MockTTSEngine tts_engine;
    auto tts_res = tts_engine.synthesize("Odysseus ko task assign kar diya gaya hai.", {});
    assert(tts_res.is_ok());

    // User interrupts
    session_mgr.trigger_barge_in();
    assert(session_mgr.current_state() == voice::session::VoiceState::Listening);

    session_mgr.end_session();
    std::cout << "[PASS] End-to-end reference voice flow verified successfully!\n";
    return 0;
}
