#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../runtime/agent/agent_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_secret_scrubbing_runtime ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    AgentController controller(gateway, pe);

    std::string raw_goal = "Connect with password = \"MySecretPassword999\" and token Bearer myVerySecretTokenXYZ";

    AgentRequest req;
    req.request_id = "req_secret_scrub";
    req.goal = raw_goal;

    auto res = controller.execute_goal(req);

    // 1. Inspect telemetry records for req_secret_scrub
    auto tel_records = AgentTelemetryCollector::instance().records_for_task("req_secret_scrub");
    assert(!tel_records.empty());

    for (const auto& r : tel_records) {
        assert(r.details.find("MySecretPassword999") == std::string::npos);
        assert(r.details.find("myVerySecretTokenXYZ") == std::string::npos);
    }
    std::cout << "[PASS] Telemetry scrubbed: raw password/token absent from all A0-A13 records.\n";

    // 2. Inspect Audit Journal
    auto audit_entries = controller.audit_journal()->entries_for_request("req_secret_scrub");
    for (const auto& a : audit_entries) {
        assert(a.safe_arguments.find("MySecretPassword999") == std::string::npos);
        assert(a.safe_arguments.find("myVerySecretTokenXYZ") == std::string::npos);
    }
    std::cout << "[PASS] Audit Journal scrubbed: raw secrets absent from journal arguments.\n";

    // 3. Inspect Task Memory
    auto mem_entries = controller.memory()->entries();
    for (const auto& m : mem_entries) {
        assert(m.content.find("MySecretPassword999") == std::string::npos);
        assert(m.content.find("myVerySecretTokenXYZ") == std::string::npos);
    }
    std::cout << "[PASS] Task Memory scrubbed: raw secrets absent from memory storage.\n";

    std::cout << "[PASS] RUNTIME_SECRET_SCRUBBING = TRUE\n";
    return 0;
}
