#include "capability_registry.hpp"

namespace vani::runtime {

CapabilityRegistry::CapabilityRegistry() = default;
CapabilityRegistry::~CapabilityRegistry() = default;

contracts::Result<void> CapabilityRegistry::register_capability(const CapabilityDescriptor& descriptor) {
    if (descriptor.id.empty()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::ValidationError,
            "Capability ID cannot be empty",
            "vani.runtime.capability_registry",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = registry_.find(descriptor.id);
    if (it != registry_.end()) {
        // Check for version conflict
        if (it->second.version == descriptor.version && it->second.provider_id != descriptor.provider_id) {
            return contracts::Result<void>::err(
                contracts::ErrorCode::AlreadyExists,
                "Capability version conflict for " + descriptor.id + " (" + descriptor.version.to_string() + ")",
                "vani.runtime.capability_registry",
                false,
                contracts::ErrorCategory::Validation
            );
        }
    }

    registry_[descriptor.id] = descriptor;
    return contracts::Result<void>::ok();
}

contracts::Result<void> CapabilityRegistry::unregister_capability(const contracts::CapabilityId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (registry_.erase(id) == 0) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Capability not found: " + id,
            "vani.runtime.capability_registry",
            false,
            contracts::ErrorCategory::Validation
        );
    }
    return contracts::Result<void>::ok();
}

std::optional<CapabilityDescriptor> CapabilityRegistry::get_capability(const contracts::CapabilityId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = registry_.find(id);
    if (it != registry_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<CapabilityDescriptor> CapabilityRegistry::list_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CapabilityDescriptor> list;
    list.reserve(registry_.size());
    for (const auto& [id, cap] : registry_) {
        list.push_back(cap);
    }
    return list;
}

std::vector<CapabilityDescriptor> CapabilityRegistry::list_by_type(CapabilityType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CapabilityDescriptor> list;
    for (const auto& [id, cap] : registry_) {
        if (cap.type == type) {
            list.push_back(cap);
        }
    }
    return list;
}

std::vector<CapabilityDescriptor> CapabilityRegistry::list_available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CapabilityDescriptor> list;
    for (const auto& [id, cap] : registry_) {
        if (cap.availability == CapabilityAvailability::Available) {
            list.push_back(cap);
        }
    }
    return list;
}

contracts::Result<void> CapabilityRegistry::set_availability(
    const contracts::CapabilityId& id,
    CapabilityAvailability availability
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = registry_.find(id);
    if (it == registry_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Capability not found: " + id,
            "vani.runtime.capability_registry",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.availability = availability;
    return contracts::Result<void>::ok();
}

size_t CapabilityRegistry::count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.size();
}

bool CapabilityRegistry::has_capability(const contracts::CapabilityId& id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.find(id) != registry_.end();
}

} // namespace vani::runtime
