#pragma once

#include "capability_descriptor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>

namespace vani::contracts {

enum class PlatformSupportStatus : uint8_t {
    Supported,
    Partial,
    Unavailable,
    RequiresPermission
};

[[nodiscard]] constexpr std::string_view to_string(PlatformSupportStatus s) noexcept {
    switch (s) {
        case PlatformSupportStatus::Supported: return "SUPPORTED";
        case PlatformSupportStatus::Partial: return "PARTIAL";
        case PlatformSupportStatus::Unavailable: return "UNAVAILABLE";
        case PlatformSupportStatus::RequiresPermission: return "REQUIRES_PERMISSION";
        default: return "UNAVAILABLE";
    }
}

struct CapabilityPlatformEntry {
    CapabilityId capability_id;
    std::string category;
    PlatformSupportStatus windows{PlatformSupportStatus::Supported};
    PlatformSupportStatus linux_{PlatformSupportStatus::Supported};
    PlatformSupportStatus macos{PlatformSupportStatus::Supported};
    std::vector<std::string> required_permissions;
    RiskLevel risk_level{RiskLevel::Low};
    Reversibility reversibility{Reversibility::Reversible};
};

class PlatformSupportMatrix {
public:
    static PlatformSupportMatrix& instance() {
        static PlatformSupportMatrix matrix;
        return matrix;
    }

    [[nodiscard]] const std::vector<CapabilityPlatformEntry>& entries() const noexcept {
        return entries_;
    }

    [[nodiscard]] PlatformSupportStatus query(const CapabilityId& cap_id, PlatformId platform) const {
        for (const auto& entry : entries_) {
            if (entry.capability_id == cap_id) {
                switch (platform) {
                    case PlatformId::Windows: return entry.windows;
                    case PlatformId::Linux: return entry.linux_;
                    case PlatformId::MacOS: return entry.macos;
                    case PlatformId::Universal: return PlatformSupportStatus::Supported;
                }
            }
        }
        return PlatformSupportStatus::Unavailable;
    }

    void register_entry(CapabilityPlatformEntry entry) {
        entries_.push_back(std::move(entry));
    }

private:
    PlatformSupportMatrix() {
        initialize_standard_matrix();
    }

    void initialize_standard_matrix() {
        // Applications
        entries_.push_back({
            "system.application.open", "application",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.application.open"}, RiskLevel::Medium, Reversibility::Reversible
        });
        entries_.push_back({
            "system.application.close", "application",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.application.close"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "system.application.focus", "application",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.application.focus"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "system.application.list", "application",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });

        // Processes
        entries_.push_back({
            "system.process.list", "process",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "system.process.inspect", "process",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "system.process.start", "process",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.process.start"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "system.process.stop", "process",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.process.stop"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "system.process.terminate", "process",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.process.terminate"}, RiskLevel::High, Reversibility::Irreversible
        });

        // Filesystem
        entries_.push_back({
            "filesystem.read", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.read"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.write", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.write"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "filesystem.create", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.write"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.move", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.write"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.copy", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.write"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.rename", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.write"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.delete", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.delete"}, RiskLevel::High, Reversibility::Reversible // Safe trash by default
        });
        entries_.push_back({
            "filesystem.search", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.read"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "filesystem.watch", "filesystem",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"filesystem.read"}, RiskLevel::Low, Reversibility::Reversible
        });

        // Terminal
        entries_.push_back({
            "terminal.execute", "terminal",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"terminal.execute"}, RiskLevel::High, Reversibility::PartiallyReversible
        });

        // Browser
        entries_.push_back({
            "browser.open", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.navigate", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.search", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.get_page", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.screenshot", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.extract", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.access"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "browser.click", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.interact"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "browser.type", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.interact"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "browser.select", "browser",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"browser.interact"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });

        // Windows
        entries_.push_back({
            "window.list", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "window.focus", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"window.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "window.resize", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"window.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "window.minimize", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"window.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "window.maximize", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"window.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "window.close", "window",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"window.control"}, RiskLevel::Medium, Reversibility::PartiallyReversible
        });

        // Input
        entries_.push_back({
            "input.key.press", "input",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"input.inject"}, RiskLevel::High, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "input.key.type", "input",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"input.inject"}, RiskLevel::High, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "input.mouse.click", "input",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"input.inject"}, RiskLevel::High, Reversibility::PartiallyReversible
        });
        entries_.push_back({
            "input.mouse.move", "input",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"input.inject"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "input.mouse.scroll", "input",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"input.inject"}, RiskLevel::Low, Reversibility::Reversible
        });

        // Clipboard
        entries_.push_back({
            "clipboard.read", "clipboard",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"clipboard.read"}, RiskLevel::Medium, Reversibility::Reversible
        });
        entries_.push_back({
            "clipboard.write", "clipboard",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"clipboard.write"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "clipboard.clear", "clipboard",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"clipboard.write"}, RiskLevel::Low, Reversibility::Reversible
        });

        // Screen
        entries_.push_back({
            "screen.capture", "screen",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"screen.capture"}, RiskLevel::Medium, Reversibility::Reversible
        });

        // Display
        entries_.push_back({
            "display.list", "display",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "display.get_brightness", "display",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Partial, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "display.set_brightness", "display",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Partial, PlatformSupportStatus::Supported,
            {"display.control"}, RiskLevel::Low, Reversibility::Reversible
        });

        // Media
        entries_.push_back({
            "media.play", "media",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"media.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "media.pause", "media",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"media.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "media.stop", "media",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"media.control"}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "media.volume", "media",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"media.control"}, RiskLevel::Low, Reversibility::Reversible
        });

        // System State
        entries_.push_back({
            "system.get_state", "system_state",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });

        // Network
        entries_.push_back({
            "system.network.read", "network",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {}, RiskLevel::Low, Reversibility::Reversible
        });
        entries_.push_back({
            "system.network.modify", "network",
            PlatformSupportStatus::RequiresPermission, PlatformSupportStatus::RequiresPermission, PlatformSupportStatus::RequiresPermission,
            {"network.modify"}, RiskLevel::Critical, Reversibility::PartiallyReversible
        });

        // Power
        entries_.push_back({
            "system.lock", "power",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.power"}, RiskLevel::Medium, Reversibility::Reversible
        });
        entries_.push_back({
            "system.sleep", "power",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.power"}, RiskLevel::Medium, Reversibility::Reversible
        });
        entries_.push_back({
            "system.restart", "power",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.power"}, RiskLevel::Critical, Reversibility::Irreversible
        });
        entries_.push_back({
            "system.shutdown", "power",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"system.power"}, RiskLevel::Critical, Reversibility::Irreversible
        });

        // Notification
        entries_.push_back({
            "system.notification.send", "notification",
            PlatformSupportStatus::Supported, PlatformSupportStatus::Supported, PlatformSupportStatus::Supported,
            {"notification.send"}, RiskLevel::Low, Reversibility::Reversible
        });
    }

    std::vector<CapabilityPlatformEntry> entries_;
};

} // namespace vani::contracts
