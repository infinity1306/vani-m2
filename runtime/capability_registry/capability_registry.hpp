#pragma once

#include "capability.hpp"
#include "../../contracts/common/result.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <optional>

namespace vani::runtime {

class CapabilityRegistry {
public:
    CapabilityRegistry();
    ~CapabilityRegistry();

    contracts::Result<void> register_capability(const CapabilityDescriptor& descriptor);
    contracts::Result<void> unregister_capability(const contracts::CapabilityId& id);

    [[nodiscard]] std::optional<CapabilityDescriptor> get_capability(const contracts::CapabilityId& id) const;
    [[nodiscard]] std::vector<CapabilityDescriptor> list_all() const;
    [[nodiscard]] std::vector<CapabilityDescriptor> list_by_type(CapabilityType type) const;
    [[nodiscard]] std::vector<CapabilityDescriptor> list_available() const;

    contracts::Result<void> set_availability(
        const contracts::CapabilityId& id,
        CapabilityAvailability availability
    );

    [[nodiscard]] size_t count() const noexcept;
    [[nodiscard]] bool has_capability(const contracts::CapabilityId& id) const noexcept;

private:
    mutable std::mutex mutex_;
    std::unordered_map<contracts::CapabilityId, CapabilityDescriptor> registry_;
};

using CapabilityRegistryPtr = std::shared_ptr<CapabilityRegistry>;

} // namespace vani::runtime
