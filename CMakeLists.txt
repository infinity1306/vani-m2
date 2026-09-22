cmake_minimum_required(VERSION 3.20)
project(VANI_M2
    VERSION 2.0.0
    DESCRIPTION "VANI Mark 2 — Local-First AI Operating Layer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs and static analyzers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings and hygiene flags
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -Wno-missing-field-initializers)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/contracts
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/voice
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/observability
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks
    "C:/Users/youri/tools/sherpa-onnx/include"
)

link_directories(
    "C:/Users/youri/tools/sherpa-onnx/lib"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Core static library for VANI runtime, audio, voice, capabilities, adapters & contracts
add_library(vani_core STATIC
    runtime/lifecycle/lifecycle_manager.cpp
    runtime/event_bus/event_bus.cpp
    runtime/capability_registry/capability_registry.cpp
    runtime/task_manager/task_manager.cpp
    runtime/task_manager/recovery_manager.cpp
    runtime/task_manager/timeout_manager.cpp
    runtime/policy/policy_engine.cpp
    runtime/permissions/permission_service.cpp
    runtime/router/router.cpp
    runtime/scheduler/scheduler.cpp
    runtime/watchdog/watchdog.cpp
    runtime/audit/audit_service.cpp
    runtime/session/session_manager.cpp
    runtime/context/context_manager.cpp
    runtime/core/resource_manager.cpp
    runtime/core/runtime.cpp
    audio/input/miniaudio_audio_input.cpp
    audio/preprocessing/audio_preprocessor.cpp
    adapters/vad/real_silero_vad_adapter.cpp
    adapters/stt/real_sherpa_stt_adapter.cpp
    adapters/stt/sherpa_sensevoice_adapter.cpp
    adapters/stt/sherpa_whisper_adapter.cpp
    adapters/wakeword/real_sherpa_kws_adapter.cpp
    adapters/wakeword/dedicated_vani_wakeword_adapter.cpp
    voice/wakeword/wakeword_engine_factory.cpp
    voice/pipeline/gated_voice_pipeline.cpp
    voice/pipeline/end_to_end_voice_loop.cpp
    voice/stt/stt_manager.cpp
    voice/stt/stt_engine_factory.cpp
    voice/normalization/language_normalizer.cpp
    voice/vocabulary/vocabulary_engine.cpp
    voice/entities/entity_resolver.cpp
    voice/intent/intent_preparer.cpp
    voice/session/voice_session_manager.cpp
    voice/tts/tts_manager.cpp
    voice/tts/tts_engine_factory.cpp
    adapters/tts/real_piper_tts_adapter.cpp
    adapters/tts/real_matcha_tts_adapter.cpp
    adapters/tts/windows_sapi_tts_adapter.cpp
    audio/output/miniaudio_audio_output.cpp
    capabilities/system/applications/application_registry.cpp
    capabilities/system/applications/application_manager.cpp
    capabilities/system/processes/process_manager.cpp
    capabilities/system/filesystem/path_security.cpp
    capabilities/system/filesystem/file_transaction.cpp
    capabilities/system/filesystem/file_watcher.cpp
    capabilities/system/filesystem/filesystem_manager.cpp
    capabilities/system/terminal/command_policy_evaluator.cpp
    capabilities/system/terminal/terminal_executor.cpp
    capabilities/system/projects/project_context.cpp
    capabilities/system/browser/browser_manager.cpp
    capabilities/system/windows/window_manager.cpp
    capabilities/system/input/input_manager.cpp
    capabilities/system/clipboard/clipboard_manager.cpp
    capabilities/system/screen/screen_capture_manager.cpp
    capabilities/system/display/display_manager.cpp
    capabilities/system/media/media_manager.cpp
    capabilities/system/system_state/system_state_provider.cpp
    capabilities/system/network/network_manager.cpp
    capabilities/system/power/power_manager.cpp
    capabilities/system/notifications/notification_manager.cpp
    capabilities/system/journal/system_action_journal.cpp
    capabilities/system/gateway/tool_gateway.cpp
    adapters/system/windows/windows_system_adapter.cpp
    adapters/system/linux/linux_system_adapter.cpp
    adapters/system/macos/macos_system_adapter.cpp
    adapters/system/mock/mock_system_adapter.cpp
    runtime/agent/plan_validator.cpp
    runtime/agent/policy_boundary.cpp
    runtime/agent/postcondition_verifier.cpp
    runtime/agent/execution_scheduler.cpp
    runtime/agent/replanner.cpp
    runtime/agent/agent_memory.cpp
    runtime/agent/agent_model_provider.cpp
    runtime/agent/complexity_classifier.cpp
    runtime/agent/agent_audit_journal.cpp
    runtime/agent/agent_controller.cpp
    runtime/agent/ollama_model_provider.cpp
    observability/logger.cpp
    observability/health_service.cpp
    config/config_manager.cpp
)

target_include_directories(vani_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(vani_core PUBLIC sherpa-onnx-c-api)
if(WIN32)
    target_link_libraries(vani_core PUBLIC psapi ole32 shell32 winhttp)
endif()

# Main VANI Daemon application
add_executable(vani-runtime
    apps/vani-runtime/main.cpp
)
target_link_libraries(vani-runtime PRIVATE vani_core)

# Main VANI CLI application
add_executable(vani-cli
    apps/vani-cli/main.cpp
)
target_link_libraries(vani-cli PRIVATE vani_core)

# Main VANI Gateway application
add_executable(vani-gateway
    apps/vani-gateway/main.cpp
)
target_link_libraries(vani-gateway PRIVATE vani_core)

# Phase 3 Voice Pipeline Benchmark
add_executable(benchmark_voice_pipeline
    benchmarks/benchmark_voice_pipeline.cpp
)
target_link_libraries(benchmark_voice_pipeline PRIVATE vani_core)

# Phase 3 Audio Unit Tests
add_executable(test_phase3_audio
    tests/unit/test_phase3_audio.cpp
)
target_link_libraries(test_phase3_audio PRIVATE vani_core)

# Phase 3 Voice Pipeline Unit Tests
add_executable(test_phase3_voice_pipeline
    tests/unit/test_phase3_voice_pipeline.cpp
)
target_link_libraries(test_phase3_voice_pipeline PRIVATE vani_core)

# Phase 3 End-to-End Reference Flow Test
add_executable(test_phase3_reference_flow
    tests/integration/test_phase3_reference_flow.cpp
)
target_link_libraries(test_phase3_reference_flow PRIVATE vani_core)

# Phase 4 Capabilities Benchmark
add_executable(benchmark_phase4_capabilities
    benchmarks/benchmark_phase4_capabilities.cpp
)
target_link_libraries(benchmark_phase4_capabilities PRIVATE vani_core)

# Phase 4 Capabilities Unit Tests
add_executable(test_phase4_capabilities
    tests/unit/test_phase4_capabilities.cpp
)
target_link_libraries(test_phase4_capabilities PRIVATE vani_core)

# Phase 4 Security Tests
add_executable(test_phase4_security
    tests/security/test_phase4_security.cpp
)
target_link_libraries(test_phase4_security PRIVATE vani_core)

# Phase 4 Reference Flows Test
add_executable(test_phase4_reference_flows
    tests/integration/test_phase4_reference_flows.cpp
)
target_link_libraries(test_phase4_reference_flows PRIVATE vani_core)

# Phase 5A Real STT Benchmark
add_executable(benchmark_real_stt
    benchmarks/benchmark_real_stt.cpp
)
target_link_libraries(benchmark_real_stt PRIVATE vani_core)

# Phase 5A Real Voice Integration Test
add_executable(test_phase5a_real_voice
    tests/integration/test_phase5a_real_voice.cpp
)
target_link_libraries(test_phase5a_real_voice PRIVATE vani_core)

# Phase 5B Live Physical Microphone & Quality Validator
add_executable(live_mic_validator
    benchmarks/live_mic_validator.cpp
)
target_link_libraries(live_mic_validator PRIVATE vani_core)

# Phase 5C STT Model Bake-Off Benchmark
add_executable(benchmark_stt_bakeoff
    benchmarks/benchmark_stt_bakeoff.cpp
)
target_link_libraries(benchmark_stt_bakeoff PRIVATE vani_core)

# Phase 5D Whisper Production Stress Validation
add_executable(benchmark_whisper_production_validation
    benchmarks/benchmark_whisper_production_validation.cpp
)
target_link_libraries(benchmark_whisper_production_validation PRIVATE vani_core)

# Phase 6A Wake-Word Detection & Audio Gating Benchmark
add_executable(benchmark_wakeword_gating
    benchmarks/benchmark_wakeword_gating.cpp
)
target_link_libraries(benchmark_wakeword_gating PRIVATE vani_core)

# Phase 6B Dedicated VANI Wake-Word Model Benchmark
add_executable(benchmark_phase6b_vani_model
    benchmarks/benchmark_phase6b_vani_model.cpp
)
target_link_libraries(benchmark_phase6b_vani_model PRIVATE vani_core)

# Phase 6C Offline TTS Bake-Off Benchmark
add_executable(benchmark_phase6c_tts
    benchmarks/benchmark_phase6c_tts.cpp
)
target_link_libraries(benchmark_phase6c_tts PRIVATE vani_core)

# Phase 6D End-to-End Voice Loop Benchmark
add_executable(benchmark_phase6d_voice_loop
    benchmarks/benchmark_phase6d_voice_loop.cpp
)
target_link_libraries(benchmark_phase6d_voice_loop PRIVATE vani_core)

# Phase 6E Live Human Microphone & Real OS Validation Benchmark
add_executable(benchmark_phase6e_live_voice
    benchmarks/benchmark_phase6e_live_voice.cpp
)
target_link_libraries(benchmark_phase6e_live_voice PRIVATE vani_core)

# Testing
enable_testing()
add_subdirectory(tests)
