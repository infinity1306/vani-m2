#pragma once

#include <string>
#include <vector>

namespace vani::benchmarks {

struct BenchmarkTestCase {
    std::string id;
    std::string raw_spoken_text;
    std::string expected_normalized_text;
    std::string category; // "Hinglish", "Technical", "Commands", "Names", "FastSpeech"
    std::string expected_route;
};

inline std::vector<BenchmarkTestCase> get_hinglish_benchmark_suite() {
    return {
        {
            .id = "TC-HING-001",
            .raw_spoken_text = "mera react js wala project rum karde",
            .expected_normalized_text = "mera React.js wala project run kar de",
            .category = "Hinglish",
            .expected_route = "agent.odysseus"
        },
        {
            .id = "TC-TECH-002",
            .raw_spoken_text = "git hub se latest code pull karke type script build chala",
            .expected_normalized_text = "GitHub se latest code pull karke TypeScript build chala",
            .category = "Technical",
            .expected_route = "agent.odysseus"
        },
        {
            .id = "TC-APP-003",
            .raw_spoken_text = "vs code kholo aur odysus ko bol bugs fix kare",
            .expected_normalized_text = "VS Code kholo aur Odysseus ko bol bugs fix kare",
            .category = "App names",
            .expected_route = "agent.odysseus"
        },
        {
            .id = "TC-RES-004",
            .raw_spoken_text = "hermes agent se 2026 ke research papers summarize karwa",
            .expected_normalized_text = "Hermes agent se 2026 ke research papers summarize karwa",
            .category = "Commands",
            .expected_route = "agent.hermes"
        },
        {
            .id = "TC-GREET-005",
            .raw_spoken_text = "hello vani kaise ho sab theek",
            .expected_normalized_text = "hello VANI kaise ho sab theek",
            .category = "Names",
            .expected_route = "runtime.dialogue"
        }
    };
}

} // namespace vani::benchmarks
