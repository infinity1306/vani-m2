#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include <string>
#include <vector>

namespace vani::runtime::agent {

struct ClassificationDecision {
    contracts::ComplexityRoute route{contracts::ComplexityRoute::FastPath};
    std::string matched_trigger;
    std::string reason;
};

class ComplexityClassifier {
public:
    ComplexityClassifier();
    ~ComplexityClassifier() = default;

    [[nodiscard]] ClassificationDecision classify(
        const std::string& raw_transcript,
        const std::string& normalized_transcript,
        const std::string& detected_intent
    ) const;

private:
    std::vector<std::string> agent_conjunction_triggers_;
    std::vector<std::string> agent_conditional_triggers_;
    std::vector<std::string> agent_workflow_triggers_;
};

} // namespace vani::runtime::agent
