#include "../../runtime/agent/agent_memory.hpp"
#include <iostream>
#include <cassert>

using namespace vani::runtime::agent;

void test_memory_observations_and_compaction() {
    // Memory with small token budget to test compaction
    ShortTermTaskMemory memory(50); // ~200 characters

    vani::contracts::AgentObservation obs1;
    obs1.step_id = "s1";
    obs1.success = true;
    obs1.evidence = "Chrome launched successfully with PID 1234";

    vani::contracts::AgentObservation obs2;
    obs2.step_id = "s2";
    obs2.success = true;
    obs2.evidence = "Browser navigated to https://google.com";

    memory.add_observation(obs1);
    memory.add_observation(obs2);

    assert(memory.entry_count() == 2);
    assert(memory.estimated_tokens() > 0);

    // Add many entries to force context compaction
    for (int i = 0; i < 10; ++i) {
        memory.add_entry("entry_" + std::to_string(i), "Long output details describing step progress in detail.");
    }

    // After compaction, earlier entries are compacted into a single summary entry
    auto entries = memory.entries();
    bool has_compacted_summary = false;
    for (const auto& e : entries) {
        if (e.key == "compacted_summary") has_compacted_summary = true;
    }
    assert(has_compacted_summary);

    std::cout << "[PASS] test_memory_observations_and_compaction\n";
}

void test_secret_and_credential_scrubbing() {
    // Verify that sensitive credentials are never stored plaintext
    std::string text_with_password = "User input: password = \"superSecret123\" for login";
    std::string scrubbed_pw = ShortTermTaskMemory::scrub_sensitive_data(text_with_password);
    assert(scrubbed_pw.find("superSecret123") == std::string::npos);
    assert(scrubbed_pw.find("[REDACTED]") != std::string::npos);

    std::string text_with_token = "Authorization: Bearer abcd1234efgh5678ijkl90";
    std::string scrubbed_tok = ShortTermTaskMemory::scrub_sensitive_data(text_with_token);
    assert(scrubbed_tok.find("abcd1234efgh5678ijkl90") == std::string::npos);
    assert(scrubbed_tok.find("[REDACTED_TOKEN]") != std::string::npos);

    std::string text_with_gh = "GitHub token is ghp_123456789012345678901234567890123456";
    std::string scrubbed_gh = ShortTermTaskMemory::scrub_sensitive_data(text_with_gh);
    assert(scrubbed_gh.find("ghp_123456789012345678901234567890123456") == std::string::npos);
    assert(scrubbed_gh.find("[REDACTED_GITHUB_TOKEN]") != std::string::npos);

    std::cout << "[PASS] test_secret_and_credential_scrubbing\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_memory ===\n";
    test_memory_observations_and_compaction();
    test_secret_and_credential_scrubbing();
    std::cout << "All memory and context tests passed successfully.\n";
    return 0;
}
