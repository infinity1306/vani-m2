#include "stt_engine_factory.hpp"
#include "../../adapters/stt/real_sherpa_stt_adapter.hpp"
#include "../../adapters/stt/sherpa_sensevoice_adapter.hpp"
#include "../../adapters/stt/sherpa_whisper_adapter.hpp"
#include "../../adapters/stt/mock_stt_adapter.hpp"
#include <algorithm>

namespace vani::voice::stt {

std::string STTEngineFactory::provider_type_to_string(STTProviderType type) {
    switch (type) {
        case STTProviderType::SherpaZipformerEn: return "sherpa_zipformer_en";
        case STTProviderType::SherpaSenseVoice: return "sensevoice";
        case STTProviderType::SherpaWhisperTiny: return "whisper_tiny";
        case STTProviderType::Mock: return "mock";
        default: return "unknown";
    }
}

STTProviderType STTEngineFactory::string_to_provider_type(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "sherpa_zipformer_en" || lower == "zipformer" || lower == "zipformer_en") {
        return STTProviderType::SherpaZipformerEn;
    } else if (lower == "sensevoice" || lower == "sense_voice" || lower == "sherpa_sensevoice") {
        return STTProviderType::SherpaSenseVoice;
    } else if (lower == "whisper_tiny" || lower == "whisper" || lower == "sherpa_whisper") {
        return STTProviderType::SherpaWhisperTiny;
    } else if (lower == "mock") {
        return STTProviderType::Mock;
    }
    return STTProviderType::SherpaZipformerEn; // default baseline
}

contracts::STTEnginePtr STTEngineFactory::create_engine(
    STTProviderType type,
    const std::string& custom_model_path
) {
    switch (type) {
        case STTProviderType::SherpaZipformerEn: {
            adapters::stt::RealSherpaSTTAdapter::Options opt;
            if (!custom_model_path.empty()) {
                opt.encoder_path = custom_model_path + "/encoder-epoch-99-avg-1.int8.onnx";
                opt.decoder_path = custom_model_path + "/decoder-epoch-99-avg-1.int8.onnx";
                opt.joiner_path  = custom_model_path + "/joiner-epoch-99-avg-1.onnx";
                opt.tokens_path  = custom_model_path + "/tokens.txt";
            }
            return std::make_shared<adapters::stt::RealSherpaSTTAdapter>(opt);
        }
        case STTProviderType::SherpaSenseVoice: {
            adapters::stt::SherpaSenseVoiceAdapter::Options opt;
            if (!custom_model_path.empty()) {
                opt.model_path  = custom_model_path + "/model.int8.onnx";
                opt.tokens_path = custom_model_path + "/tokens.txt";
            }
            return std::make_shared<adapters::stt::SherpaSenseVoiceAdapter>(opt);
        }
        case STTProviderType::SherpaWhisperTiny: {
            adapters::stt::SherpaWhisperAdapter::Options opt;
            if (!custom_model_path.empty()) {
                opt.encoder_path = custom_model_path + "/tiny-encoder.int8.onnx";
                opt.decoder_path = custom_model_path + "/tiny-decoder.int8.onnx";
                opt.tokens_path  = custom_model_path + "/tiny-tokens.txt";
            }
            return std::make_shared<adapters::stt::SherpaWhisperAdapter>(opt);
        }
        case STTProviderType::Mock:
            return std::make_shared<adapters::MockSTTAdapter>();
        default:
            return std::make_shared<adapters::stt::RealSherpaSTTAdapter>();
    }
}

contracts::STTEnginePtr STTEngineFactory::create_engine_by_name(
    const std::string& provider_name,
    const std::string& custom_model_path
) {
    return create_engine(string_to_provider_type(provider_name), custom_model_path);
}

} // namespace vani::voice::stt
