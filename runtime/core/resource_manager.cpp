#include "resource_manager.hpp"

namespace vani::runtime {

ResourceManager::ResourceManager() {
    current_snapshot_ = SystemResourceSnapshot{
        .cpu_usage_percent = 15,
        .ram_total_mb = 32768,
        .ram_used_mb = 8192,
        .gpu_usage_percent = 5,
        .vram_total_mb = 16384,
        .vram_used_mb = 2048,
        .disk_free_mb = 524288,
        .battery_percent = 100,
        .is_charging = true,
        .has_active_network = true
    };
}

ResourceManager::~ResourceManager() = default;

SystemResourceSnapshot ResourceManager::get_current_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_snapshot_;
}

void ResourceManager::update_snapshot(const SystemResourceSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_snapshot_ = snapshot;
}

bool ResourceManager::has_sufficient_resources(
    uint64_t required_ram_mb,
    bool requires_gpu,
    uint64_t required_vram_mb
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto available_ram = (current_snapshot_.ram_total_mb > current_snapshot_.ram_used_mb)
        ? (current_snapshot_.ram_total_mb - current_snapshot_.ram_used_mb)
        : 0;

    if (available_ram < required_ram_mb) {
        return false;
    }

    if (requires_gpu) {
        const auto available_vram = (current_snapshot_.vram_total_mb > current_snapshot_.vram_used_mb)
            ? (current_snapshot_.vram_total_mb - current_snapshot_.vram_used_mb)
            : 0;
        if (available_vram < required_vram_mb) {
            return false;
        }
    }

    return true;
}

} // namespace vani::runtime
