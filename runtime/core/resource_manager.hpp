#pragma once

#include "../../contracts/common/result.hpp"
#include <string>
#include <memory>
#include <mutex>

namespace vani::runtime {

struct SystemResourceSnapshot {
    uint8_t cpu_usage_percent{0};
    uint64_t ram_total_mb{0};
    uint64_t ram_used_mb{0};
    uint8_t gpu_usage_percent{0};
    uint64_t vram_total_mb{0};
    uint64_t vram_used_mb{0};
    uint64_t disk_free_mb{0};
    uint8_t battery_percent{100};
    bool is_charging{true};
    bool has_active_network{true};
};

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    [[nodiscard]] SystemResourceSnapshot get_current_snapshot() const;
    void update_snapshot(const SystemResourceSnapshot& snapshot);

    [[nodiscard]] bool has_sufficient_resources(
        uint64_t required_ram_mb,
        bool requires_gpu = false,
        uint64_t required_vram_mb = 0
    ) const;

private:
    mutable std::mutex mutex_;
    SystemResourceSnapshot current_snapshot_;
};

using ResourceManagerPtr = std::shared_ptr<ResourceManager>;

} // namespace vani::runtime
