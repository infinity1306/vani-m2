#pragma once

#include "../../../contracts/system/system_contracts.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <memory>

namespace vani::capabilities::system {

class ApplicationRegistry {
public:
    ApplicationRegistry();
    ~ApplicationRegistry() = default;

    void register_application(const contracts::ApplicationMetadata& app);
    void unregister_application(const std::string& application_id);

    [[nodiscard]] std::optional<contracts::ApplicationMetadata> resolve(const std::string& name_or_alias) const;
    [[nodiscard]] std::vector<contracts::ApplicationMetadata> list_all() const;
    [[nodiscard]] size_t count() const noexcept;

    void load_standard_registry();

private:
    [[nodiscard]] static std::string normalize(std::string_view str);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, contracts::ApplicationMetadata> apps_;
    std::unordered_map<std::string, std::string> alias_to_app_id_;
};

using ApplicationRegistryPtr = std::shared_ptr<ApplicationRegistry>;

} // namespace vani::capabilities::system
