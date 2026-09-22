#include "wakeword_engine_factory.hpp"
#include "../../adapters/wakeword/dedicated_vani_wakeword_adapter.hpp"
#include "../../adapters/wakeword/real_sherpa_kws_adapter.hpp"
#include "../../adapters/wakeword/open_wakeword_adapter.hpp"
#include <algorithm>

namespace vani::voice::wakeword {

std::string WakeWordEngineFactory::provider_type_to_string(WakeWordProviderType type) {
    switch (type) {
        case WakeWordProviderType::DedicatedVani: return "dedicated_vani";
        case WakeWordProviderType::SherpaKWS: return "sherpa_kws";
        case WakeWordProviderType::OpenWakeWord: return "openwakeword";
        case WakeWordProviderType::Mock: return "mock";
        default: return "unknown";
    }
}

WakeWordProviderType WakeWordEngineFactory::string_to_provider_type(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "dedicated_vani" || lower == "vani" || lower == "custom_vani") {
        return WakeWordProviderType::DedicatedVani;
    } else if (lower == "sherpa_kws" || lower == "sherpa" || lower == "zipformer_kws") {
        return WakeWordProviderType::SherpaKWS;
    } else if (lower == "openwakeword" || lower == "open_wakeword") {
        return WakeWordProviderType::OpenWakeWord;
    } else if (lower == "mock") {
        return WakeWordProviderType::Mock;
    }
    return WakeWordProviderType::DedicatedVani;
}

audio::wakeword::WakeWordEnginePtr WakeWordEngineFactory::create_engine(
    WakeWordProviderType type,
    const audio::wakeword::WakeWordConfig& config
) {
    switch (type) {
        case WakeWordProviderType::DedicatedVani:
            return std::make_shared<adapters::wakeword::DedicatedVaniWakeWordAdapter>(
                adapters::wakeword::DedicatedVaniWakeWordAdapter::Options{},
                config
            );
        case WakeWordProviderType::SherpaKWS:
            return std::make_shared<adapters::wakeword::RealSherpaKwsAdapter>(
                adapters::wakeword::RealSherpaKwsAdapter::Options{},
                config
            );
        case WakeWordProviderType::OpenWakeWord:
            return std::make_shared<adapters::wakeword::OpenWakeWordAdapter>(config);
        case WakeWordProviderType::Mock:
            return std::make_shared<adapters::wakeword::MockWakeWordEngine>(config);
        default:
            return std::make_shared<adapters::wakeword::DedicatedVaniWakeWordAdapter>(
                adapters::wakeword::DedicatedVaniWakeWordAdapter::Options{},
                config
            );
    }
}

audio::wakeword::WakeWordEnginePtr WakeWordEngineFactory::create_engine_by_name(
    const std::string& provider_name,
    const audio::wakeword::WakeWordConfig& config
) {
    return create_engine(string_to_provider_type(provider_name), config);
}

} // namespace vani::voice::wakeword
