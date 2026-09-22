#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::contracts {

struct IntegrationManifest {
    std::string id;
    SemanticVersion version{1, 0, 0};
    std::string name;
    std::string category; // e.g. "Development", "Communication", "LMS", "CRM"
    std::string description;
    std::vector<std::string> exposed_capabilities;
    std::vector<std::string> required_permissions;
    bool requires_network{true};
};

class Integration {
public:
    virtual ~Integration() = default;

    [[nodiscard]] virtual IntegrationManifest manifest() const = 0;

    virtual Result<void> initialize() = 0;

    virtual Result<void> shutdown() = 0;

    [[nodiscard]] virtual bool is_healthy() const = 0;
};

using IntegrationPtr = std::shared_ptr<Integration>;

} // namespace vani::contracts
