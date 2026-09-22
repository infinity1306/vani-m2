#include "intent_preparer.hpp"
#include <chrono>
#include <algorithm>

namespace vani::voice::intent {

IntentPreparer::IntentPreparer(entities::EntityResolverPtr /*resolver*/) {}

Utterance IntentPreparer::prepare(
    const std::string& session_id,
    const normalization::NormalizedText& normalized,
    const std::vector<entities::ResolvedEntity>& entities
) const {
    Utterance utt;
    utt.session_id = session_id;
    utt.timestamp_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    utt.raw_transcript = normalized.raw_text;
    utt.normalized_transcript = normalized.normalized_text;
    utt.language = normalized.language;
    utt.entities = entities;

    std::string lower = normalized.normalized_text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Formulate candidate intents
    if (lower.find("band karo") != std::string::npos ||
        lower.find("close") != std::string::npos ||
        lower.find("band kar") != std::string::npos ||
        lower.find("exit") != std::string::npos ||
        lower.find("quit") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "application.close",
            .score = 0.95f,
            .suggested_agent = "vani_core",
            .required_tools = {"applications"}
        });
        utt.preferred_route = "runtime.fast_path";
    } else if (lower.find("localhost") != std::string::npos ||
               lower.find("github") != std::string::npos ||
               lower.find("8000") != std::string::npos ||
               lower.find("url") != std::string::npos ||
               lower.find("website") != std::string::npos ||
               lower.find("endpoint") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "browser.open_url",
            .score = 0.94f,
            .suggested_agent = "vani_core",
            .required_tools = {"browser"}
        });
        utt.preferred_route = "runtime.fast_path";
    } else if (lower.find("chrome") != std::string::npos ||
               lower.find("youtube") != std::string::npos ||
               lower.find("kholo") != std::string::npos ||
               lower.find("launch") != std::string::npos ||
               lower.find("open") != std::string::npos ||
               lower.find("start") != std::string::npos ||
               lower.find("chalu") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "application.launch",
            .score = 0.95f,
            .suggested_agent = "vani_core",
            .required_tools = {"applications"}
        });
        utt.preferred_route = "runtime.fast_path";
    } else if (lower.find("volume") != std::string::npos ||
               lower.find("awaz") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "media.volume",
            .score = 0.95f,
            .suggested_agent = "vani_core",
            .required_tools = {"media"}
        });
        utt.preferred_route = "runtime.fast_path";
    } else if (lower.find("status") != std::string::npos ||
               lower.find("server check") != std::string::npos ||
               lower.find("memory and cpu") != std::string::npos ||
               lower.find("check karo") != std::string::npos ||
               lower.find("batao") != std::string::npos ||
               lower.find("inspect") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "system.status",
            .score = 0.93f,
            .suggested_agent = "vani_core",
            .required_tools = {"system_state"}
        });
        utt.preferred_route = "runtime.fast_path";
    } else if (lower.find("run") != std::string::npos ||
               lower.find("build") != std::string::npos ||
               lower.find("code") != std::string::npos ||
               lower.find("react") != std::string::npos ||
               lower.find("dev") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "software.execute_workflow",
            .score = 0.92f,
            .suggested_agent = "odysseus",
            .required_tools = {"terminal", "filesystem", "git"}
        });
        utt.preferred_route = "agent.odysseus";
    } else if (lower.find("research") != std::string::npos ||
               lower.find("find") != std::string::npos ||
               lower.find("search") != std::string::npos ||
               lower.find("paper") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "intelligence.synthesize_research",
            .score = 0.90f,
            .suggested_agent = "hermes",
            .required_tools = {"browser", "semantic_search", "vector_db"}
        });
        utt.preferred_route = "agent.hermes";
    } else if (lower.find("hello") != std::string::npos ||
               lower.find("namaste") != std::string::npos ||
               lower.find("kaise ho") != std::string::npos) {
        utt.candidate_intents.push_back({
            .intent_name = "system.conversational_greeting",
            .score = 0.98f,
            .suggested_agent = "vani_core",
            .required_tools = {}
        });
        utt.preferred_route = "runtime.dialogue";
    } else {
        utt.candidate_intents.push_back({
            .intent_name = "system.general_command",
            .score = 0.70f,
            .suggested_agent = "vani_core",
            .required_tools = {}
        });
        utt.preferred_route = "runtime.router";
    }

    utt.confidence = {
        .audio_confidence = 1.0f,
        .stt_confidence = normalized.confidence,
        .language_confidence = 0.95f,
        .normalization_confidence = 0.95f,
        .entity_confidence = entities.empty() ? 0.8f : 0.98f,
        .intent_confidence = utt.candidate_intents.empty() ? 0.5f : utt.candidate_intents[0].score
    };

    return utt;
}

} // namespace vani::voice::intent
