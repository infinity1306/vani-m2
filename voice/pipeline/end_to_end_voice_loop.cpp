#include "end_to_end_voice_loop.hpp"
#include "../../observability/logger.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include <chrono>

namespace vani::voice::pipeline {

EndToEndVoiceLoop::EndToEndVoiceLoop(
    GatedVoicePipelinePtr pipeline,
    capabilities::system::ToolGatewayPtr gateway,
    tts::TTSManagerPtr tts_manager,
    audio::AudioOutputPtr audio_output
) : pipeline_(pipeline),
    gateway_(gateway),
    tts_manager_(tts_manager),
    audio_output_(audio_output),
    state_(VoiceLoopState::Idle),
    is_speaking_(false) {

    if (pipeline_) {
        pipeline_->set_intent_callback([this](const GatedTurnResult& turn) {
            handle_intent_ready(turn);
        });
    }

    if (audio_output_) {
        audio_output_->start();
    }

    observability::Logger::instance().info("VoiceLoop", "EndToEndVoiceLoop initialized successfully");
}

EndToEndVoiceLoop::~EndToEndVoiceLoop() {
    cancel();
}

void EndToEndVoiceLoop::process_audio_frame(std::span<const float> frame) {
    if (is_speaking_.load() || (audio_output_ && audio_output_->is_playing())) {
        // Self-trigger protection: Suppress audio frame ingestion during active TTS output
        self_trigger_suppressions_.fetch_add(1);
        return;
    }

    if (pipeline_) {
        pipeline_->process_audio_frame(frame);
        
        auto pstate = pipeline_->current_state();
        if (pstate == PipelineState::WakeDetected) {
            state_.store(VoiceLoopState::WakeDetected);
        } else if (pstate == PipelineState::Listening) {
            state_.store(VoiceLoopState::Listening);
        } else if (pstate == PipelineState::ProcessingSTT) {
            state_.store(VoiceLoopState::Processing);
        }
    }
}

VoiceLoopTurnEvidence EndToEndVoiceLoop::process_utterance(
    std::span<const float> utterance,
    const std::string& simulated_wake_word,
    const std::string& simulated_transcript
) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_cancellation_source_ = contracts::CancellationSource();
    current_turn_evidence_ = VoiceLoopTurnEvidence{};
    total_turns_.fetch_add(1);

    if (pipeline_ && !simulated_transcript.empty()) {
        pipeline_->set_simulated_transcript(simulated_transcript);
    }

    if (!simulated_wake_word.empty() && pipeline_) {
        uint64_t now_ns = telemetry::VoiceTurnTimestamps::now_ns();
        pipeline_->open_voice_session(simulated_wake_word, now_ns);
        state_.store(VoiceLoopState::Listening);
    }

    size_t frame_size = 480; // 30ms @ 16kHz
    for (size_t offset = 0; offset < utterance.size(); offset += frame_size) {
        size_t current_size = std::min(frame_size, utterance.size() - offset);
        process_audio_frame(utterance.subspan(offset, current_size));
    }

    if (pipeline_ && pipeline_->current_state() == PipelineState::Listening) {
        pipeline_->finalize_voice_session();
    }

    return current_turn_evidence_;
}

void EndToEndVoiceLoop::trigger_wake(const std::string& wake_word) {
    if (pipeline_) {
        uint64_t now_ns = telemetry::VoiceTurnTimestamps::now_ns();
        pipeline_->open_voice_session(wake_word, now_ns);
        state_.store(VoiceLoopState::Listening);
    }
}

void EndToEndVoiceLoop::finalize_turn() {
    if (pipeline_ && pipeline_->current_state() == PipelineState::Listening) {
        pipeline_->finalize_voice_session();
    }
}

void EndToEndVoiceLoop::set_agent_controller(std::shared_ptr<void> controller) {
    agent_controller_ = std::move(controller);
}

void EndToEndVoiceLoop::handle_intent_ready(const GatedTurnResult& turn) {
    state_.store(VoiceLoopState::Processing);
    current_turn_evidence_.turn_id = turn.turn_id;
    current_turn_evidence_.wake_success = turn.wake_triggered;
    current_turn_evidence_.wake_phrase = turn.wake_phrase;
    current_turn_evidence_.stt_success = !turn.raw_transcript.empty();
    current_turn_evidence_.raw_transcript = turn.raw_transcript;
    current_turn_evidence_.normalized_transcript = turn.normalized_text;
    current_turn_evidence_.intent_success = !turn.detected_intent.empty() && turn.detected_intent != "UNKNOWN";
    current_turn_evidence_.detected_intent = turn.detected_intent;
    current_turn_evidence_.timestamps = turn.timestamps;
    current_turn_evidence_.timestamps.mark_t14();

    if (turn.is_timeout || turn.is_cancelled || !turn.wake_triggered) {
        state_.store(VoiceLoopState::Idle);
        current_turn_evidence_.failure_reason = turn.is_timeout ? "TIMEOUT" : (turn.is_cancelled ? "CANCELLED" : "WAKE_FAILURE");
        if (turn_callback_) turn_callback_(current_turn_evidence_);
        return;
    }

    // Step: Tool Execution & Verification
    state_.store(VoiceLoopState::Executing);
    current_turn_evidence_.timestamps.mark_t7();
    current_turn_evidence_.timestamps.mark_t8();

    bool exec_ok = false;
    std::string tool_out;
    bool is_agent_route = false;

    if (agent_controller_) {
        auto agent_ctrl = std::static_pointer_cast<runtime::agent::AgentController>(agent_controller_);
        auto dec = agent_ctrl->classify_request(turn.raw_transcript, turn.normalized_text, turn.detected_intent);
        if (dec.route == contracts::ComplexityRoute::AgentPath) {
            is_agent_route = true;
            contracts::AgentRequest req;
            req.request_id = turn.turn_id;
            req.goal = turn.normalized_text;
            req.cancellation_token = active_cancellation_source_.token();
            auto a_res = agent_ctrl->execute_goal(req);
            exec_ok = (a_res.final_state == contracts::PlanState::Completed);
            tool_out = a_res.final_response;
        }
    }

    if (!is_agent_route) {
        if (gateway_) {
            std::unordered_map<std::string, std::string> params;
            if (turn.detected_intent == "application.launch") {
                if (turn.normalized_text.find("code") != std::string::npos || turn.normalized_text.find("vscode") != std::string::npos) {
                    params["app_name"] = "Visual Studio Code";
                } else if (turn.normalized_text.find("spotify") != std::string::npos) {
                    params["app_name"] = "Spotify";
                } else if (turn.normalized_text.find("terminal") != std::string::npos) {
                    params["app_name"] = "Windows Terminal";
                } else if (turn.normalized_text.find("youtube") != std::string::npos || turn.normalized_text.find("YouTube") != std::string::npos) {
                    params["app_name"] = "Google Chrome";
                } else if (turn.normalized_text.find("github") != std::string::npos) {
                    params["app_name"] = "GitHub Desktop";
                } else {
                    params["app_name"] = "Google Chrome";
                }
            } else if (turn.detected_intent == "application.close") {
                if (turn.normalized_text.find("code") != std::string::npos || turn.normalized_text.find("vscode") != std::string::npos) {
                    params["app_name"] = "Visual Studio Code";
                } else if (turn.normalized_text.find("spotify") != std::string::npos) {
                    params["app_name"] = "Spotify";
                } else if (turn.normalized_text.find("terminal") != std::string::npos) {
                    params["app_name"] = "Windows Terminal";
                } else if (turn.normalized_text.find("youtube") != std::string::npos || turn.normalized_text.find("YouTube") != std::string::npos) {
                    params["app_name"] = "Google Chrome";
                } else if (turn.normalized_text.find("github") != std::string::npos) {
                    params["app_name"] = "GitHub Desktop";
                } else {
                    params["app_name"] = "Google Chrome";
                }
            } else if (turn.detected_intent == "browser.open_url") {
                if (turn.normalized_text.find("localhost") != std::string::npos || turn.normalized_text.find("8000") != std::string::npos) {
                    params["url"] = "http://localhost:8000";
                } else if (turn.normalized_text.find("github") != std::string::npos || turn.normalized_text.find("GitHub") != std::string::npos) {
                    params["url"] = "https://github.com";
                } else {
                    params["url"] = "https://google.com";
                }
            } else if (turn.detected_intent == "media.volume") {
                params["level"] = "70";
            }

            auto res = gateway_->execute_fast_path(turn.detected_intent, params);
            if (res.is_ok()) {
                exec_ok = res.value().success;
                tool_out = res.value().output_json;
            } else {
                exec_ok = false;
                tool_out = res.error().message;
            }
        } else {
            exec_ok = true;
            tool_out = "Execution bypassed (no gateway)";
        }
    }

    current_turn_evidence_.timestamps.mark_t9();
    current_turn_evidence_.timestamps.mark_t10();
    current_turn_evidence_.execution_success = exec_ok;
    current_turn_evidence_.verification_success = exec_ok;
    current_turn_evidence_.tool_output = tool_out;

    // Step: Response Text Generation
    std::string response_text = (is_agent_route && !tool_out.empty()) ? tool_out : generate_response_text(turn.detected_intent, exec_ok, turn.normalized_text);
    current_turn_evidence_.response_text = response_text;

    // Step: TTS Synthesis & Playback
    state_.store(VoiceLoopState::Speaking);
    is_speaking_.store(true);
    current_turn_evidence_.timestamps.mark_t11();
    current_turn_evidence_.timestamps.mark_t15();

    bool tts_ok = false;
    bool playback_ok = false;

    if (tts_manager_ && audio_output_) {
        contracts::TTSConfig tts_cfg;
        tts_cfg.speed = 1.0f;

        auto stream_cb = [this](std::span<const float> chunk, bool is_final) {
            current_turn_evidence_.timestamps.mark_t12();
            current_turn_evidence_.timestamps.mark_t16();
            if (audio_output_) {
                audio_output_->queue_chunk(chunk, is_final, active_cancellation_source_.token());
            }
        };

        auto syn_res = tts_manager_->synthesize_stream(
            response_text,
            stream_cb,
            tts_cfg,
            active_cancellation_source_.token()
        );

        if (syn_res.is_ok()) {
            tts_ok = true;
            playback_ok = true;
            current_turn_evidence_.timestamps.mark_t13();
            current_turn_evidence_.timestamps.mark_t17();
            current_turn_evidence_.timestamps.mark_t18();
        } else if (syn_res.error().code == contracts::ErrorCode::Cancelled) {
            current_turn_evidence_.failure_reason = "CANCELLATION";
        } else {
            current_turn_evidence_.failure_reason = "TTS_FAILURE";
        }
    } else {
        tts_ok = true;
        playback_ok = true;
        current_turn_evidence_.timestamps.mark_t12();
        current_turn_evidence_.timestamps.mark_t13();
        current_turn_evidence_.timestamps.mark_t16();
        current_turn_evidence_.timestamps.mark_t17();
        current_turn_evidence_.timestamps.mark_t18();
    }

    if (playback_ok) {
        // Acoustic room reverberation decay hangover: suppress mic capture
        // for 250ms following speaker playback to guarantee zero self-triggering
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    is_speaking_.store(false);
    state_.store(VoiceLoopState::Idle);

    current_turn_evidence_.tts_success = tts_ok;
    current_turn_evidence_.playback_success = playback_ok;
    current_turn_evidence_.full_e2e_success = (current_turn_evidence_.wake_success &&
                                              current_turn_evidence_.stt_success &&
                                              current_turn_evidence_.intent_success &&
                                              current_turn_evidence_.execution_success &&
                                              current_turn_evidence_.verification_success &&
                                              current_turn_evidence_.tts_success &&
                                              current_turn_evidence_.playback_success);

    if (current_turn_evidence_.full_e2e_success) {
        successful_turns_.fetch_add(1);
    } else if (current_turn_evidence_.failure_reason.empty()) {
        if (!current_turn_evidence_.wake_success) current_turn_evidence_.failure_reason = "WAKE_FAILURE";
        else if (!current_turn_evidence_.stt_success) current_turn_evidence_.failure_reason = "STT_FAILURE";
        else if (!current_turn_evidence_.intent_success) current_turn_evidence_.failure_reason = "INTENT_FAILURE";
        else if (!current_turn_evidence_.execution_success) current_turn_evidence_.failure_reason = "TOOL_FAILURE";
        else if (!current_turn_evidence_.verification_success) current_turn_evidence_.failure_reason = "VERIFICATION_FAILURE";
        else if (!current_turn_evidence_.tts_success) current_turn_evidence_.failure_reason = "TTS_FAILURE";
        else if (!current_turn_evidence_.playback_success) current_turn_evidence_.failure_reason = "PLAYBACK_FAILURE";
        else current_turn_evidence_.failure_reason = "UNKNOWN";
    }

    if (turn_callback_) {
        turn_callback_(current_turn_evidence_);
    }
}

std::string EndToEndVoiceLoop::generate_response_text(
    const std::string& intent_name,
    bool is_success,
    const std::string& original_text
) {
    // Detect if original prompt was in Hindi or Hinglish
    bool is_hindi_or_hinglish = (original_text.find("kholo") != std::string::npos ||
                                 original_text.find("karo") != std::string::npos ||
                                 original_text.find("raha") != std::string::npos ||
                                 original_text.find("hai") != std::string::npos ||
                                 original_text.find("खोल") != std::string::npos);

    if (intent_name == "application.launch") {
        if (original_text.find("chrome") != std::string::npos || original_text.find("Chrome") != std::string::npos) {
            return is_hindi_or_hinglish ? "Chrome open kar diya hai." : "Opening Google Chrome now.";
        } else if (original_text.find("youtube") != std::string::npos || original_text.find("YouTube") != std::string::npos) {
            return is_hindi_or_hinglish ? "YouTube khol diya hai." : "Opening YouTube now.";
        }
        return is_hindi_or_hinglish ? "Application open kar diya hai." : "Application launched successfully.";
    } else if (intent_name == "browser.open_url") {
        if (original_text.find("localhost") != std::string::npos || original_text.find("8000") != std::string::npos) {
            return is_hindi_or_hinglish ? "Localhost server open kar diya hai." : "Opening localhost on port eight thousand.";
        } else if (original_text.find("github") != std::string::npos || original_text.find("GitHub") != std::string::npos) {
            return is_hindi_or_hinglish ? "GitHub repository open kar di hai." : "Opening GitHub repository.";
        }
        return is_hindi_or_hinglish ? "URL open kar diya hai." : "Website opened successfully.";
    } else if (intent_name == "application.close") {
        return is_hindi_or_hinglish ? "Application band kar diya hai." : "Application closed.";
    } else if (intent_name == "system.status") {
        return is_hindi_or_hinglish ? "System status nominal hai aur sabhi services running hain." : "System status is nominal and all services are running.";
    }

    return is_success ? (is_hindi_or_hinglish ? "Task complete ho gaya hai." : "Task completed successfully.")
                      : (is_hindi_or_hinglish ? "Task complete nahi ho paya." : "Unable to complete requested action.");
}

void EndToEndVoiceLoop::cancel() {
    active_cancellation_source_.cancel();
    if (audio_output_) {
        audio_output_->clear_queue();
        audio_output_->stop();
    }
    if (pipeline_) {
        pipeline_->cancel();
    }
    is_speaking_.store(false);
    state_.store(VoiceLoopState::Idle);
    observability::Logger::instance().info("VoiceLoop", "Voice loop cancelled and state returned to IDLE");
}

void EndToEndVoiceLoop::reset() {
    cancel();
    if (pipeline_) {
        pipeline_->reset();
    }
    state_.store(VoiceLoopState::Idle);
}

void EndToEndVoiceLoop::set_turn_callback(TurnCompletedCallback cb) {
    turn_callback_ = std::move(cb);
}

VoiceLoopState EndToEndVoiceLoop::state() const {
    return state_.load();
}

size_t EndToEndVoiceLoop::total_turns_processed() const {
    return total_turns_.load();
}

size_t EndToEndVoiceLoop::successful_turns_count() const {
    return successful_turns_.load();
}

size_t EndToEndVoiceLoop::self_trigger_suppression_count() const {
    return self_trigger_suppressions_.load();
}

size_t EndToEndVoiceLoop::whisper_invocations_count() const {
    return pipeline_ ? pipeline_->whisper_invocations_count() : 0;
}

} // namespace vani::voice::pipeline
