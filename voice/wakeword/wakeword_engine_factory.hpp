#pragma once

#include "../../audio/wakeword/wakeword_engine.hpp"
#include <string>
#include <memory>

namespace vani::voice::wakeword {

enum class WakeWordProviderType {
    DedicatedVani,
    SherpaKWS,
    OpenWakeWord,
    Mock
};

class WakeWordEngineFactory {
public:
    static audio::wakeword::WakeWordEnginePtr create_engine(
        WakeWordProviderType type,
        const audio::wakeword::WakeWordConfig& config = {}
    );

    static audio::wakeword::WakeWordEnginePtr create_engine_by_name(
        const std::string& provider_name,
        const audio::wakeword::WakeWordConfig& config = {}
    );

    static std::string provider_type_to_string(WakeWordProviderType type);
    static WakeWordProviderType string_to_provider_type(const std::string& name);
};

} // namespace vani::voice::wakeword
