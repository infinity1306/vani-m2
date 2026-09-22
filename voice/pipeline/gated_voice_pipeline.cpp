#include "gated_voice_pipeline.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace vani::voice::pipeline {

GatedVoicePipeline::GatedVoicePipeline(
    audio::wakeword::WakeWordEnginePtr wakeword_engine,
    contracts::STTEnginePtr stt_engine,
    const GatedPipelineConfig& config
) : wakeword_(std::move(wakeword_engine)),
    stt_(std::move(stt_engine)),
    config_(config),
    vad_detector_(config.vad_config) {
    
    pre_roll_capacity_samples_ = (config_.sample_rate * config_.pre_roll_ms) / 1000;
    if (pre_roll_capacity_samples_ == 0) pre_roll_capacity_samples_ = 9600;
    pre_roll_buffer_.resize(pre_roll_capacity_samples_, 0.0f);
}

GatedVoicePipeline::~GatedVoicePipeline() {
    cancel();
}

void GatedVoicePipeline::set_intent_callback(IntentCallback cb) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    intent_callback_ = std::move(cb);
}

PipelineState GatedVoicePipeline::current_state() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_;
}

size_t GatedVoicePipeline::whisper_invocations_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return whisper_invocations_;
}

size_t GatedVoicePipeline::wake_detections_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return wake_detections_;
}

size_t GatedVoicePipeline::timeout_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return timeouts_;
}

void GatedVoicePipeline::process_audio_frame(std::span<const float> frame) {
    if (frame.empty()) return;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto now_clock = std::chrono::steady_clock::now();
    uint64_t now_ns = telemetry::VoiceTurnTimestamps::now_ns();

    // 1. Always maintain the bounded circular pre-roll buffer in Idle
    if (state_ == PipelineState::Idle) {
        for (float s : frame) {
            pre_roll_buffer_[pre_roll_write_pos_] = s;
            pre_roll_write_pos_ = (pre_roll_write_pos_ + 1) % pre_roll_capacity_samples_;
            if (pre_roll_write_pos_ == 0) {
                pre_roll_full_ = true;
            }
        }

        // Track ambient noise floor adaptively during Idle
        vad_detector_.process_frame(frame, /*is_idle=*/true);

        // Run Wake-Word Detection on the incoming frame
        if (wakeword_ && wakeword_->is_enabled()) {
            auto det = wakeword_->process(frame);
            if (det.detected) {
                open_voice_session(det.wake_word, now_ns);
            }
        }
        return;
    }

    // 2. Active Listening / STT Streaming State
    if (state_ == PipelineState::Listening || state_ == PipelineState::ProcessingSTT) {
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_clock - session_start_time_).count();

        // Stream audio frame into STT Engine
        if (stt_) {
            stt_->push_audio(frame);
        }

        // Adaptive Voice Activity / Silence Endpointing
        auto vad_res = vad_detector_.process_frame(frame, /*is_idle=*/false);

        // Detect active speech in incoming frame via adaptive VAD or interim transcript
        if (vad_res.is_speech || !last_partial_text_.empty()) {
            speech_detected_in_session_ = true;
            last_speech_time_ = now_clock;
        }

        // 1. Natural turn finalization when speech ends and trailing silence elapsed
        if (speech_detected_in_session_) {
            auto silence_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_clock - last_speech_time_).count();
            if (silence_ms >= config_.max_trailing_silence_ms) {
                finalize_voice_session();
                return;
            }
            // Max utterance cap: 20 seconds of continuous speech
            if (elapsed_ms > 20000) {
                finalize_voice_session();
                return;
            }
        } else {
            // 2. Timeout if no speech is detected within the initial listening window
            if (elapsed_ms > config_.listen_timeout_ms) {
                timeouts_++;
                state_ = PipelineState::Timeout;
                if (stt_) {
                    stt_->stop_stream();
                }

                GatedTurnResult timeout_result{
                    .turn_id = current_turn_id_,
                    .wake_triggered = true,
                    .wake_phrase = current_wake_word_,
                    .raw_transcript = "",
                    .normalized_text = "",
                    .detected_intent = "UNKNOWN",
                    .timestamps = current_timestamps_,
                    .is_timeout = true,
                    .is_cancelled = false
                };

                if (intent_callback_) {
                    intent_callback_(timeout_result);
                }

                reset();
                return;
            }
        }
    }
}

void GatedVoicePipeline::open_voice_session(const std::string& wake_word, uint64_t wake_time_ns) {
    wake_detections_++;
    state_ = PipelineState::Listening;
    current_wake_word_ = wake_word;
    current_turn_id_ = "turn-" + std::to_string(wake_detections_);
    session_start_time_ = std::chrono::steady_clock::now();
    last_speech_time_ = session_start_time_;
    speech_detected_in_session_ = false;
    last_partial_text_.clear();
    final_stt_text_.clear();
    vad_detector_.reset_session();

    current_timestamps_ = telemetry::VoiceTurnTimestamps{};
    current_timestamps_.turn_id = current_turn_id_;
    current_timestamps_.t0_audio_capture_ns = wake_time_ns;
    current_timestamps_.t2_wake_word_detected_ns = wake_time_ns; // T14/T2 wake detection
    current_timestamps_.t14_wake_detected_ns = wake_time_ns;

    if (!stt_) return;

    // Start STT stream
    whisper_invocations_++;
    contracts::STTConfig stt_cfg{
        .sample_rate_hz = config_.sample_rate,
        .channels = 1,
        .language_preference = "auto",
        .enable_interim_results = true
    };

    stt_->start_stream(stt_cfg, [this](const contracts::STTTranscript& t) {
        std::lock_guard<std::recursive_mutex> lk(mutex_);
        if (!t.is_final) {
            if (!current_timestamps_.t3_first_stt_partial_ns) {
                current_timestamps_.mark_t3();
            }
            last_partial_text_ = t.text;
        } else {
            current_timestamps_.mark_t4();
            final_stt_text_ = t.text;
        }
    });

    // Flush pre-roll buffer to STT to prevent initial word clipping
    std::vector<float> linear_pre_roll;
    linear_pre_roll.reserve(pre_roll_capacity_samples_);

    if (pre_roll_full_) {
        // Read from write_pos_ to end, then from 0 to write_pos_
        linear_pre_roll.insert(linear_pre_roll.end(), pre_roll_buffer_.begin() + pre_roll_write_pos_, pre_roll_buffer_.end());
        linear_pre_roll.insert(linear_pre_roll.end(), pre_roll_buffer_.begin(), pre_roll_buffer_.begin() + pre_roll_write_pos_);
    } else {
        linear_pre_roll.insert(linear_pre_roll.end(), pre_roll_buffer_.begin(), pre_roll_buffer_.begin() + pre_roll_write_pos_);
    }

    if (!linear_pre_roll.empty()) {
        stt_->push_audio(linear_pre_roll);
    }
}

void GatedVoicePipeline::set_simulated_transcript(const std::string& transcript) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    simulated_transcript_ = transcript;
}

void GatedVoicePipeline::finalize_voice_session() {
    if (state_ == PipelineState::Idle) return;

    if (stt_) {
        stt_->stop_stream();
    }

    if (!current_timestamps_.t3_first_stt_partial_ns) current_timestamps_.mark_t3();
    if (!current_timestamps_.t4_final_stt_result_ns) current_timestamps_.mark_t4();
    current_timestamps_.mark_t5();
    std::string text_to_use;
    if (!simulated_transcript_.empty()) {
        text_to_use = simulated_transcript_;
    } else {
        text_to_use = final_stt_text_.empty() ? last_partial_text_ : final_stt_text_;
    }

    contracts::STTTranscript stt_input{
        .text = text_to_use,
        .is_final = true
    };

    auto norm_result = normalizer_.normalize(stt_input);
    normalization::NormalizedText norm_text;
    if (norm_result.is_ok()) {
        norm_text = norm_result.value();
    } else {
        norm_text.raw_text = stt_input.text;
        norm_text.normalized_text = stt_input.text;
    }

    current_timestamps_.mark_t6();
    auto utt = intent_preparer_.prepare("gated_session", norm_text);

    std::string detected_intent = utt.candidate_intents.empty() ? "UNKNOWN" : utt.candidate_intents[0].intent_name;

    GatedTurnResult result{
        .turn_id = current_turn_id_,
        .wake_triggered = true,
        .wake_phrase = current_wake_word_,
        .raw_transcript = stt_input.text,
        .normalized_text = norm_text.normalized_text,
        .detected_intent = detected_intent,
        .timestamps = current_timestamps_,
        .is_timeout = false,
        .is_cancelled = false
    };

    if (intent_callback_) {
        intent_callback_(result);
    }

    reset();
}

void GatedVoicePipeline::cancel() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (state_ != PipelineState::Idle) {
        if (stt_) {
            stt_->stop_stream();
        }
        state_ = PipelineState::Cancelled;
        
        GatedTurnResult cancelled_result{
            .turn_id = current_turn_id_,
            .wake_triggered = true,
            .wake_phrase = current_wake_word_,
            .raw_transcript = "",
            .normalized_text = "",
            .detected_intent = "CANCELLED",
            .timestamps = current_timestamps_,
            .is_timeout = false,
            .is_cancelled = true
        };

        if (intent_callback_) {
            intent_callback_(cancelled_result);
        }

        reset();
    }
}

void GatedVoicePipeline::reset() {
    state_ = PipelineState::Idle;
    pre_roll_write_pos_ = 0;
    pre_roll_full_ = false;
    std::fill(pre_roll_buffer_.begin(), pre_roll_buffer_.end(), 0.0f);
    current_turn_id_.clear();
    current_wake_word_.clear();
    last_partial_text_.clear();
    final_stt_text_.clear();
    simulated_transcript_.clear();
    vad_detector_.reset_session();
    if (wakeword_) {
        wakeword_->reset();
    }
}

float GatedVoicePipeline::noise_floor_rms() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.noise_floor_rms();
}

float GatedVoicePipeline::adaptive_vad_threshold() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.adaptive_vad_threshold();
}

float GatedVoicePipeline::current_frame_rms() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.current_frame_rms();
}

uint64_t GatedVoicePipeline::speech_frame_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.speech_frame_count();
}

uint64_t GatedVoicePipeline::silence_frame_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.silence_frame_count();
}

uint64_t GatedVoicePipeline::speech_start_timestamp_ns() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.speech_start_timestamp_ns();
}

uint64_t GatedVoicePipeline::speech_end_timestamp_ns() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return vad_detector_.speech_end_timestamp_ns();
}

bool GatedVoicePipeline::speech_detected_in_session() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return speech_detected_in_session_;
}

} // namespace vani::voice::pipeline
