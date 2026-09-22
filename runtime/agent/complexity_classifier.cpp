#include "complexity_classifier.hpp"
#include <algorithm>

namespace vani::runtime::agent {

ComplexityClassifier::ComplexityClassifier() {
    agent_conjunction_triggers_ = {
        " aur ",
        " and ",
        " then ",
        " phir ",
        " uske baad ",
        " ke baad ",
        " followed by ",
        " also "
    };

    agent_conditional_triggers_ = {
        " if ",
        " agar ",
        " check and if ",
        " check if ",
        " dekho agar ",
        " in case ",
        " unless "
    };

    agent_workflow_triggers_ = {
        "find then",
        "compare then",
        "identify karke",
        "summary bana",
        "error dekh ke",
        "inspect and report",
        "locate project",
        "check build"
    };
}

ClassificationDecision ComplexityClassifier::classify(
    const std::string& raw_transcript,
    const std::string& normalized_transcript,
    const std::string& detected_intent
) const {
    ClassificationDecision dec;

    // Combine transcripts to check for multi-step triggers
    std::string text = " " + normalized_transcript + " ";
    std::string raw_lower = " " + raw_transcript + " ";
    std::transform(raw_lower.begin(), raw_lower.end(), raw_lower.begin(), ::tolower);

    // 1. Check workflow triggers
    for (const auto& trig : agent_workflow_triggers_) {
        if (text.find(trig) != std::string::npos || raw_lower.find(trig) != std::string::npos) {
            dec.route = contracts::ComplexityRoute::AgentPath;
            dec.matched_trigger = trig;
            dec.reason = "Workflow multi-step keyword matched: " + trig;
            return dec;
        }
    }

    // 2. Check conjunction triggers (multi-step action combination)
    for (const auto& trig : agent_conjunction_triggers_) {
        if (text.find(trig) != std::string::npos || raw_lower.find(trig) != std::string::npos) {
            dec.route = contracts::ComplexityRoute::AgentPath;
            dec.matched_trigger = trig;
            dec.reason = "Multi-step conjunction keyword matched: " + trig;
            return dec;
        }
    }

    // 3. Check conditional triggers
    for (const auto& trig : agent_conditional_triggers_) {
        if (text.find(trig) != std::string::npos || raw_lower.find(trig) != std::string::npos) {
            dec.route = contracts::ComplexityRoute::AgentPath;
            dec.matched_trigger = trig;
            dec.reason = "Conditional logic trigger matched: " + trig;
            return dec;
        }
    }

    // 4. Default: Fast Path for single discrete commands
    dec.route = contracts::ComplexityRoute::FastPath;
    dec.matched_trigger = "single_intent:" + (detected_intent.empty() ? "none" : detected_intent);
    dec.reason = "Single discrete intent without conjunctions or conditional logic";
    return dec;
}

} // namespace vani::runtime::agent
