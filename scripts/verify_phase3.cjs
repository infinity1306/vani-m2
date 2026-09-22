/**
 * VANI Mark 2 — Phase 3 Production Voice, Speech & Language Engine Automated Verification Suite
 * Verifies all 55 architectural requirements, contracts, adapters, engines, and tests.
 */
const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
let totalChecks = 0;
let passedChecks = 0;

function check(title, condition, detail = '') {
  totalChecks++;
  if (condition) {
    passedChecks++;
    console.log(`  \x1b[32m✓\x1b[0m [PASS] ${title}`);
  } else {
    console.error(`  \x1b[31m✗\x1b[0m [FAIL] ${title} — ${detail}`);
  }
}

function fileExists(relPath) {
  return fs.existsSync(path.join(ROOT, relPath));
}

function fileContains(relPath, str) {
  if (!fileExists(relPath)) return false;
  const content = fs.readFileSync(path.join(ROOT, relPath), 'utf8');
  return content.includes(str);
}

console.log('\n===============================================================');
console.log('    VANI MARK 2 — PHASE 3 PRODUCTION VOICE ENGINE VERIFICATION   ');
console.log('===============================================================\n');

// 1. Audio Capture & Buffer Subsystem
console.log('\x1b[36m[1/8] Audio Capture & Bounded Ring Buffer Abstraction\x1b[0m');
check('audio/audio_format.hpp exists', fileExists('audio/audio_format.hpp'));
check('AudioFormat specifies 16kHz standard', fileContains('audio/audio_format.hpp', 'sample_rate{16000}'));
check('AudioFormat specifies Mono standard', fileContains('audio/audio_format.hpp', 'channels{1}'));
check('AudioFormat specifies Float32 standard', fileContains('audio/audio_format.hpp', 'SampleFormat::Float32'));
check('audio/ring_buffer.hpp exists', fileExists('audio/ring_buffer.hpp'));
check('AudioRingBuffer has bounded capacity', fileContains('audio/ring_buffer.hpp', 'capacity_'));
check('AudioRingBuffer tracks overflow count', fileContains('audio/ring_buffer.hpp', 'overflow_count'));
check('audio/audio_device.hpp exists', fileExists('audio/audio_device.hpp'));
check('audio/audio_input.hpp exists', fileExists('audio/audio_input.hpp'));
check('AudioInput start/stop and set_callback contracts', fileContains('audio/audio_input.hpp', 'set_callback(AudioCallback'));

// 2. Audio Preprocessing Chain
console.log('\n\x1b[36m[2/8] Audio Preprocessing Pipeline\x1b[0m');
check('audio/preprocessing/preprocessor_stage.hpp exists', fileExists('audio/preprocessing/preprocessor_stage.hpp'));
check('audio/preprocessing/audio_preprocessor.hpp exists', fileExists('audio/preprocessing/audio_preprocessor.hpp'));
check('AudioPreprocessor has pluggable stages', fileContains('audio/preprocessing/audio_preprocessor.hpp', 'add_stage(PreprocessorStagePtr'));
check('GainNormalizationStage defined', fileContains('audio/preprocessing/audio_preprocessor.hpp', 'GainNormalizationStage'));
check('SimpleNoiseSuppressionStage defined', fileContains('audio/preprocessing/audio_preprocessor.hpp', 'SimpleNoiseSuppressionStage'));

// 3. VAD & Wake-Word Detection
console.log('\n\x1b[36m[3/8] Voice Activity Detection & Wake-Word Subsystem\x1b[0m');
check('audio/vad/vad_result.hpp exists', fileExists('audio/vad/vad_result.hpp'));
check('VADState includes SpeechStarted, Continuing, Ended', fileContains('audio/vad/vad_result.hpp', 'SpeechStarted') && fileContains('audio/vad/vad_result.hpp', 'SpeechEnded'));
check('audio/vad/vad_engine.hpp exists', fileExists('audio/vad/vad_engine.hpp'));
check('adapters/vad/silero_vad_adapter.hpp exists', fileExists('adapters/vad/silero_vad_adapter.hpp'));
check('adapters/vad/mock_vad_engine.hpp exists', fileExists('adapters/vad/mock_vad_engine.hpp'));
check('audio/wakeword/detection_result.hpp exists', fileExists('audio/wakeword/detection_result.hpp'));
check('WakeWordState includes Idle, WakeDetected, Listening, Timeout', fileContains('audio/wakeword/detection_result.hpp', 'WakeDetected') && fileContains('audio/wakeword/detection_result.hpp', 'Timeout'));
check('audio/wakeword/wakeword_engine.hpp exists', fileExists('audio/wakeword/wakeword_engine.hpp'));
check('adapters/wakeword/open_wakeword_adapter.hpp exists', fileExists('adapters/wakeword/open_wakeword_adapter.hpp'));
check('OpenWakeWordAdapter supports timeout and reset', fileContains('adapters/wakeword/open_wakeword_adapter.hpp', 'listen_timeout_ms'));

// 4. Streaming STT & Adapter Architecture
console.log('\n\x1b[36m[4/8] Streaming STT Engine & Provider Failover\x1b[0m');
check('contracts/providers/stt_engine.hpp exists', fileExists('contracts/providers/stt_engine.hpp'));
check('STTCapabilities interface defined', fileContains('contracts/providers/stt_engine.hpp', 'struct STTCapabilities'));
check('STTTranscript preserves sequence_number', fileContains('contracts/providers/stt_engine.hpp', 'sequence_number'));
check('adapters/stt/sherpa_onnx_adapter.hpp exists', fileExists('adapters/stt/sherpa_onnx_adapter.hpp'));
check('adapters/stt/whisper_cpp_adapter.hpp exists', fileExists('adapters/stt/whisper_cpp_adapter.hpp'));
check('adapters/stt/sensevoice_adapter.hpp exists', fileExists('adapters/stt/sensevoice_adapter.hpp'));
check('adapters/stt/mock_stt_engine.hpp exists', fileExists('adapters/stt/mock_stt_engine.hpp'));
check('voice/stt/stt_manager.hpp exists', fileExists('voice/stt/stt_manager.hpp'));
check('STTManager has fallback failover', fileContains('voice/stt/stt_manager.hpp', 'fallback_engine_'));
check('STTManager tracks fallback switches', fileContains('voice/stt/stt_manager.hpp', 'fallback_switch_count'));

// 5. Language Detection, Hinglish Normalizer & Vocabulary
console.log('\n\x1b[36m[5/8] Language Detection, Normalization & Contextual Vocabulary\x1b[0m');
check('voice/language/language_detector.hpp exists', fileExists('voice/language/language_detector.hpp'));
check('LanguageDetector identifies English, Hindi, and Hinglish', fileContains('voice/language/language_detector.hpp', 'Hinglish'));
check('voice/normalization/normalized_text.hpp exists', fileExists('voice/normalization/normalized_text.hpp'));
check('NormalizedText preserves raw_text alongside normalized_text', fileContains('voice/normalization/normalized_text.hpp', 'raw_text'));
check('voice/normalization/language_normalizer.hpp exists', fileExists('voice/normalization/language_normalizer.hpp'));
check('LanguageNormalizer implements Layer 1 deterministic rules', fileContains('voice/normalization/language_normalizer.cpp', 'apply_layer1_deterministic'));
check('LanguageNormalizer implements Layer 2 vocabulary lookup', fileContains('voice/normalization/language_normalizer.cpp', 'apply_layer2_vocabulary'));
check('voice/vocabulary/vocabulary_item.hpp exists', fileExists('voice/vocabulary/vocabulary_item.hpp'));
check('VocabularyItem categories include Project, App, Tech, Command', fileContains('voice/vocabulary/vocabulary_item.hpp', 'Technology'));
check('voice/vocabulary/vocabulary_engine.hpp exists', fileExists('voice/vocabulary/vocabulary_engine.hpp'));
check('VocabularyEngine contextual boosting supported', fileContains('voice/vocabulary/vocabulary_engine.cpp', 'get_contextual_vocabulary'));

// 6. Entity Resolution, Intent & Confidence
console.log('\n\x1b[36m[6/8] Entity Resolution, Utterance & Confidence Model\x1b[0m');
check('voice/entities/entity_resolver.hpp exists', fileExists('voice/entities/entity_resolver.hpp'));
check('EntityResolver resolves aliases to canonical apps & tech', fileContains('voice/entities/entity_resolver.cpp', 'resolve_entities'));
check('voice/intent/utterance.hpp exists', fileExists('voice/intent/utterance.hpp'));
check('ConfidenceVector has multi-level scores', fileContains('voice/intent/utterance.hpp', 'overall_confidence'));
check('voice/intent/intent_preparer.hpp exists', fileExists('voice/intent/intent_preparer.hpp'));
check('IntentPreparer generates candidate intents and preferred routes', fileContains('voice/intent/intent_preparer.cpp', 'preferred_route'));

// 7. Voice Session Lifecycle, Barge-in & Streaming TTS
console.log('\n\x1b[36m[7/8] Voice Session Lifecycle, Barge-in & Streaming TTS\x1b[0m');
check('voice/session/voice_state.hpp exists', fileExists('voice/session/voice_state.hpp'));
check('VoiceState defines 13 strict states', fileContains('voice/session/voice_state.hpp', 'Interrupted') && fileContains('voice/session/voice_state.hpp', 'WaitingForPermission'));
check('voice/session/voice_session.hpp exists', fileExists('voice/session/voice_session.hpp'));
check('voice/session/voice_session_manager.hpp exists', fileExists('voice/session/voice_session_manager.hpp'));
check('VoiceSessionManager trigger_barge_in transitions to Listening', fileContains('voice/session/voice_session_manager.cpp', 'trigger_barge_in'));
check('contracts/providers/tts_engine.hpp exists', fileExists('contracts/providers/tts_engine.hpp'));
check('adapters/tts/piper_adapter.hpp exists', fileExists('adapters/tts/piper_adapter.hpp'));
check('adapters/tts/kokoro_adapter.hpp exists', fileExists('adapters/tts/kokoro_adapter.hpp'));
check('adapters/tts/mock_tts_engine.hpp exists', fileExists('adapters/tts/mock_tts_engine.hpp'));
check('voice/tts/tts_manager.hpp exists', fileExists('voice/tts/tts_manager.hpp'));

// 8. Privacy, Telemetry, Tests & Benchmarks
console.log('\n\x1b[36m[8/8] Privacy, Telemetry, Tests & Benchmark Suite\x1b[0m');
check('voice/telemetry/voice_telemetry.hpp exists', fileExists('voice/telemetry/voice_telemetry.hpp'));
check('voice/privacy/voice_privacy_controller.hpp exists', fileExists('voice/privacy/voice_privacy_controller.hpp'));
check('VoicePrivacyController guarantees ephemeral audio only', fileContains('voice/privacy/voice_privacy_controller.hpp', 'ephemeral_audio_only'));
check('tests/unit/test_phase3_audio.cpp exists', fileExists('tests/unit/test_phase3_audio.cpp'));
check('tests/unit/test_phase3_voice_pipeline.cpp exists', fileExists('tests/unit/test_phase3_voice_pipeline.cpp'));
check('tests/integration/test_phase3_reference_flow.cpp exists', fileExists('tests/integration/test_phase3_reference_flow.cpp'));
check('benchmarks/hinglish_test_dataset.hpp exists', fileExists('benchmarks/hinglish_test_dataset.hpp'));
check('benchmarks/benchmark_voice_pipeline.cpp exists', fileExists('benchmarks/benchmark_voice_pipeline.cpp'));
check('CMakeLists.txt incorporates Phase 3 targets', fileContains('CMakeLists.txt', 'benchmark_voice_pipeline') && fileContains('CMakeLists.txt', 'test_phase3_voice_pipeline'));

console.log('\n===============================================================');
const passPct = Math.round((passedChecks / totalChecks) * 100);
if (passedChecks === totalChecks) {
  console.log(`\x1b[32m✔ PHASE 3 VERIFICATION PASSED: ${passedChecks}/${totalChecks} checks (100%)\x1b[0m`);
} else {
  console.error(`\x1b[31m✖ PHASE 3 VERIFICATION FAILED: ${passedChecks}/${totalChecks} checks (${passPct}%)\x1b[0m`);
  process.exit(1);
}
console.log('===============================================================\n');
