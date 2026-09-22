#pragma once

#include "../../audio/vad/vad_engine.hpp"

namespace vani::adapters::vad {

class MockVADEngine : public audio::vad::VADEngine {
public:
    explicit MockVADEngine(audio::vad::VADConfig config = {}) : config_(config) {}

    audio::vad::VADResult process(std::span<const float> /*audio_frame*/) override {
        return forced_result_;
    }

    void force_result(const audio::vad::VADResult& res) {
        forced_result_ = res;
    }

    void reset() override {
        forced_result_ = {};
    }

    [[nodiscard]] std::string engine_name() const override {
        return "MockVAD";
    }

    [[nodiscard]] const audio::vad::VADConfig& config() const override {
        return config_;
    }

    void set_config(const audio::vad::VADConfig& config) override {
        config_ = config;
    }

private:
    audio::vad::VADConfig config_;
    audio::vad::VADResult forced_result_{};
};

} // namespace vani::adapters::vad
