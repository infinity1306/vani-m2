#pragma once

#include "risk_level.hpp"
#include "common/version.hpp"
#include <string>
#include <vector>

namespace vani::contracts {

struct ToolManifest {
    ToolId id;
    SemanticVersion version{1, 0, 0};
    std::string name;
    std::string description;
    std::string input_schema_json;
    std::string output_schema_json;
    std::vector<std::string> required_capabilities;
    std::vector<std::string> required_permissions;
    RiskLevel risk_level{RiskLevel::Low};
    uint32_t default_timeout_ms{10000};
    bool is_reversible{false};
};

} // namespace vani::contracts
