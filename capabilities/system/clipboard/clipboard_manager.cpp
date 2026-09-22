#include "clipboard_manager.hpp"

namespace vani::capabilities::system {

ClipboardManager::ClipboardManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<contracts::ClipboardPayload> ClipboardManager::read_clipboard() {
    if (!adapter_) {
        return contracts::Result<contracts::ClipboardPayload>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->read_clipboard();
}

contracts::Result<void> ClipboardManager::write_clipboard(const std::string& text, bool sensitive) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    contracts::ClipboardPayload payload{
        contracts::ClipboardContentType::Text, text, {}, sensitive
    };
    return adapter_->write_clipboard(payload);
}

contracts::Result<void> ClipboardManager::clear_clipboard() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->clear_clipboard();
}

std::string ClipboardManager::sanitize_for_audit(const contracts::ClipboardPayload& payload) {
    std::string type_str = "Text";
    if (payload.type == contracts::ClipboardContentType::Image) type_str = "Image";
    else if (payload.type == contracts::ClipboardContentType::Files) type_str = "Files";
    else if (payload.type == contracts::ClipboardContentType::Empty) type_str = "Empty";

    return "[CLIPBOARD_CONTENT: Type=" + type_str +
           ", Length=" + std::to_string(payload.text_content.size()) +
           ", Sensitive=" + (payload.contains_sensitive_data ? "true" : "false") + "]";
}

} // namespace vani::capabilities::system
