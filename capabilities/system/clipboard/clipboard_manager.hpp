#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>

namespace vani::capabilities::system {

class ClipboardManager {
public:
    explicit ClipboardManager(adapters::system::SystemAdapterPtr adapter);
    ~ClipboardManager() = default;

    contracts::Result<contracts::ClipboardPayload> read_clipboard();
    contracts::Result<void> write_clipboard(const std::string& text, bool sensitive = false);
    contracts::Result<void> clear_clipboard();

    // Security invariant: never log or expose raw payload to log streams
    [[nodiscard]] static std::string sanitize_for_audit(const contracts::ClipboardPayload& payload);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using ClipboardManagerPtr = std::shared_ptr<ClipboardManager>;

} // namespace vani::capabilities::system
