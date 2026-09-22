#include "../../runtime/agent/complexity_classifier.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

void test_fast_path_routing() {
    ComplexityClassifier classifier;

    // Test 1: "Chrome kholo" -> Fast Path
    auto dec1 = classifier.classify("Chrome kholo", "chrome open", "application.launch");
    assert(dec1.route == ComplexityRoute::FastPath);
    assert(to_string(dec1.route) == "FAST_PATH");

    // Fast Path: Volume
    auto dec2 = classifier.classify("volume 40 percent karo", "volume 40 percent", "media.volume");
    assert(dec2.route == ComplexityRoute::FastPath);

    // Fast Path: Status
    auto dec3 = classifier.classify("system status batao", "system status", "system.status");
    assert(dec3.route == ComplexityRoute::FastPath);

    std::cout << "[PASS] test_fast_path_routing\n";
}

void test_agent_path_routing() {
    ComplexityClassifier classifier;

    // Test 2: Multi-step conjunction ("Chrome kholo aur Google open karo") -> Agent Path
    auto dec1 = classifier.classify("Chrome kholo aur Google open karo", "chrome open and google open", "application.launch");
    assert(dec1.route == ComplexityRoute::AgentPath);
    assert(to_string(dec1.route) == "AGENT_PATH");
    assert(dec1.matched_trigger.find("aur") != std::string::npos || dec1.matched_trigger.find("and") != std::string::npos);

    // Conditional trigger: "Agar Chrome open hai toh close karo"
    auto dec2 = classifier.classify("Agar Chrome open hai toh close karo", "agar chrome open hai", "application.close");
    assert(dec2.route == ComplexityRoute::AgentPath);
    assert(dec2.matched_trigger.find("agar") != std::string::npos);

    // Workflow trigger: "desktop par files identify karke summary bana"
    auto dec3 = classifier.classify("desktop par files identify karke summary bana", "desktop identify karke summary bana", "filesystem.list");
    assert(dec3.route == ComplexityRoute::AgentPath);
    assert(dec3.matched_trigger.find("identify karke") != std::string::npos || dec3.matched_trigger.find("summary bana") != std::string::npos);

    // Sequential English: "open downloads folder and check react project"
    auto dec4 = classifier.classify("open downloads folder and check react project", "open downloads folder and check react project", "folder.open");
    assert(dec4.route == ComplexityRoute::AgentPath);

    std::cout << "[PASS] test_agent_path_routing\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_fast_path_routing ===\n";
    test_fast_path_routing();
    test_agent_path_routing();
    std::cout << "All fast path routing tests passed successfully.\n";
    return 0;
}
