#pragma once

#include "../../contracts/providers/stt_engine.hpp"
#include <string>
#include <memory>

namespace vani::voice::stt {

enum class STTProviderType {
    SherpaZipformerEn,
    SherpaSenseVoice,
    SherpaWhisperTiny,
    Mock
};

class STTEngineFactory {
public:
    static contracts::STTEnginePtr create_engine(
        STTProviderType type,
        const std::string& custom_model_path = ""
    );

    static contracts::STTEnginePtr create_engine_by_name(
        const std::string& provider_name,
        const std::string& custom_model_path = ""
    );

    static std::string provider_type_to_string(STTProviderType type);
    static STTProviderType string_to_provider_type(const std::string& name);
};

} // namespace vani::voice::stt
