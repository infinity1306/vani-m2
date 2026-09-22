#include "ollama_model_provider.hpp"
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <chrono>
#include <iostream>
#include <regex>
#include <algorithm>

namespace vani::runtime::agent {

namespace {

std::string escape_json_string(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

// Lightweight JSON field extraction helpers
std::string extract_json_string_field(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re) && match.size() > 1) {
        return match.str(1);
    }
    return "";
}

uint64_t extract_json_int_field(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*([0-9]+)";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re) && match.size() > 1) {
        try {
            return std::stoull(match.str(1));
        } catch (...) {}
    }
    return 0;
}

std::wstring to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
    return ws;
}

} // namespace

OllamaAgentModelProvider::OllamaAgentModelProvider(
    std::string model_name,
    std::string host,
    int port
) : model_name_(std::move(model_name)),
    host_(std::move(host)),
    port_(port) {}

bool OllamaAgentModelProvider::is_available() const {
    return ping();
}

bool OllamaAgentModelProvider::ping() const {
    HINTERNET hSession = WinHttpOpen(L"VANI-Ping/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    // Fast 1.5s timeout for ping
    WinHttpSetTimeouts(hSession, 1500, 1500, 1500, 1500);

    HINTERNET hConnect = WinHttpConnect(hSession, to_wide(host_).c_str(), static_cast<INTERNET_PORT>(port_), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/tags", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL bSend = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    bool ok = false;
    if (bSend && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
            ok = (dwStatusCode == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

OllamaInferenceTelemetry OllamaAgentModelProvider::last_telemetry() const {
    std::lock_guard<std::mutex> lock(telemetry_mutex_);
    return last_telemetry_;
}

contracts::Result<std::string> OllamaAgentModelProvider::call_generate_api(
    const std::string& prompt,
    const std::string& request_id,
    uint32_t timeout_ms
) {
    OllamaInferenceTelemetry tel;
    tel.provider = "Ollama";
    tel.model = model_name_;
    tel.request_id = request_id;
    tel.prompt = prompt;
    tel.inference_start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::ostringstream json_payload;
    json_payload << "{\n"
                 << "  \"model\": \"" << escape_json_string(model_name_) << "\",\n"
                 << "  \"prompt\": \"" << escape_json_string(prompt) << "\",\n"
                 << "  \"stream\": false,\n"
                 << "  \"format\": \"json\"\n"
                 << "}";

    std::string payload_str = json_payload.str();

    HINTERNET hSession = WinHttpOpen(L"VANI-Agent/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        tel.error = "WinHttpOpen failed: " + std::to_string(GetLastError());
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
        return contracts::Result<std::string>::failure(contracts::ErrorCode::InternalError, tel.error);
    }

    WinHttpSetTimeouts(hSession, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    HINTERNET hConnect = WinHttpConnect(hSession, to_wide(host_).c_str(), static_cast<INTERNET_PORT>(port_), 0);
    if (!hConnect) {
        tel.error = "Cannot connect to local Ollama at " + host_ + ":" + std::to_string(port_);
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
        return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, tel.error);
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/generate", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        tel.error = "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
        return contracts::Result<std::string>::failure(contracts::ErrorCode::InternalError, tel.error);
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL bSend = WinHttpSendRequest(
        hRequest,
        headers.c_str(),
        static_cast<DWORD>(headers.length()),
        (LPVOID)payload_str.c_str(),
        static_cast<DWORD>(payload_str.length()),
        static_cast<DWORD>(payload_str.length()),
        0
    );

    if (!bSend) {
        tel.error = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
        return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, tel.error);
    }

    BOOL bRecv = WinHttpReceiveResponse(hRequest, NULL);
    if (!bRecv) {
        tel.error = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
        return contracts::Result<std::string>::failure(contracts::ErrorCode::Timeout, tel.error);
    }

    tel.first_token_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::string response_body;
    DWORD dwSize = 0;
    while (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
        std::string buffer(dwSize, '\0');
        DWORD dwDownloaded = 0;
        if (WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded)) {
            response_body.append(buffer.c_str(), dwDownloaded);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    tel.inference_complete_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    tel.latency_ms = (tel.inference_complete_ns - tel.inference_start_ns) / 1000000;
    tel.raw_response = response_body;

    // Parse token telemetry from Ollama response
    tel.prompt_token_count = extract_json_int_field(response_body, "prompt_eval_count");
    tel.output_token_count = extract_json_int_field(response_body, "eval_count");
    tel.success = (tel.output_token_count > 0);

    {
        std::lock_guard<std::mutex> lock(telemetry_mutex_);
        last_telemetry_ = tel;
    }

    // Extract the inner "response" field which contains the generated plan JSON string
    size_t resp_idx = response_body.find("\"response\":");
    if (resp_idx != std::string::npos) {
        // Find beginning of value
        size_t start = response_body.find("\"", resp_idx + 11);
        if (start != std::string::npos) {
            // It is an escaped JSON string. Extract until matching unescaped quote.
            std::string unescaped;
            bool escape = false;
            for (size_t i = start + 1; i < response_body.size(); ++i) {
                char c = response_body[i];
                if (escape) {
                    switch (c) {
                        case 'n': unescaped += '\n'; break;
                        case 't': unescaped += '\t'; break;
                        case 'r': unescaped += '\r'; break;
                        case '"': unescaped += '"'; break;
                        case '\\': unescaped += '\\'; break;
                        default: unescaped += c; break;
                    }
                    escape = false;
                } else if (c == '\\') {
                    escape = true;
                } else if (c == '"') {
                    break;
                } else {
                    unescaped += c;
                }
            }
            return contracts::Result<std::string>::success(unescaped);
        }
    }

    return contracts::Result<std::string>::success(response_body);
}

contracts::Result<contracts::AgentPlan> OllamaAgentModelProvider::parse_plan_json(
    const std::string& raw_json,
    const std::string& request_id
) {
    contracts::AgentPlan plan;
    plan.plan_id = "plan_ollama_" + request_id;
    plan.request_id = request_id;
    plan.planner_provider = "ollama:" + model_name_;
    plan.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Check if empty
    if (raw_json.empty()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::ValidationError,
            "Empty plan output from LLM"
        );
    }

    // Find steps array
    size_t steps_idx = raw_json.find("\"steps\"");
    if (steps_idx == std::string::npos) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::ValidationError,
            "Missing 'steps' array in model generated JSON"
        );
    }

    size_t array_start = raw_json.find("[", steps_idx);
    size_t array_end = raw_json.rfind("]");
    if (array_start == std::string::npos || array_end == std::string::npos || array_end < array_start) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::ValidationError,
            "Malformed 'steps' array boundaries in model JSON"
        );
    }

    std::string steps_content = raw_json.substr(array_start + 1, array_end - array_start - 1);

    // Parse each step block { ... }
    size_t pos = 0;
    int step_num = 1;
    while ((pos = steps_content.find("{", pos)) != std::string::npos) {
        size_t block_end = steps_content.find("}", pos);
        if (block_end == std::string::npos) break;

        std::string block = steps_content.substr(pos, block_end - pos + 1);
        pos = block_end + 1;

        contracts::AgentStep step;
        step.step_id = extract_json_string_field(block, "id");
        if (step.step_id.empty()) {
            step.step_id = "step_" + std::to_string(step_num);
        }

        step.capability_id = extract_json_string_field(block, "capability");
        if (step.capability_id.empty()) {
            step.capability_id = extract_json_string_field(block, "capability_id");
        }

        // Map capability to tool_id
        if (step.capability_id == "application.launch") {
            step.tool_id = "app_manager.launch";
        } else if (step.capability_id == "application.close") {
            step.tool_id = "app_manager.close";
        } else if (step.capability_id == "filesystem.write") {
            step.tool_id = "filesystem.write";
        } else if (step.capability_id == "filesystem.read") {
            step.tool_id = "filesystem.read";
        } else if (step.capability_id == "browser.open_url") {
            step.tool_id = "browser_manager.open";
        } else if (step.capability_id == "media.volume") {
            step.tool_id = "media_manager.set_volume";
        } else {
            step.tool_id = step.capability_id;
        }

        step.expected_postcondition = extract_json_string_field(block, "postcondition");
        if (step.expected_postcondition.empty()) {
            step.expected_postcondition = extract_json_string_field(block, "expected_postcondition");
        }

        // Arguments extraction
        size_t args_idx = block.find("\"arguments\"");
        if (args_idx != std::string::npos) {
            size_t a_start = block.find("{", args_idx);
            size_t a_end = block.find("}", a_start);
            if (a_start != std::string::npos && a_end != std::string::npos) {
                std::string args_block = block.substr(a_start + 1, a_end - a_start - 1);
                // Extract "key": "val"
                std::regex kv_re("\"([^\"]+)\"\\s*:\\s*\"([^\"]*)\"");
                auto kv_begin = std::sregex_iterator(args_block.begin(), args_block.end(), kv_re);
                auto kv_end = std::sregex_iterator();
                for (auto it = kv_begin; it != kv_end; ++it) {
                    step.arguments[(*it)[1].str()] = (*it)[2].str();
                }
            }
        }

        if (step.expected_postcondition.empty()) {
            if (step.capability_id == "application.launch") {
                step.expected_postcondition = "process.running:" + (step.arguments.count("app_name") ? step.arguments["app_name"] : "notepad.exe");
            } else if (step.capability_id == "application.close") {
                step.expected_postcondition = "process.terminated:" + (step.arguments.count("app_name") ? step.arguments["app_name"] : "notepad.exe");
            } else if (step.capability_id == "filesystem.write") {
                step.expected_postcondition = "file.exists:" + (step.arguments.count("path") ? step.arguments["path"] : "");
            } else if (step.capability_id == "filesystem.read") {
                step.expected_postcondition = "file.read:" + (step.arguments.count("path") ? step.arguments["path"] : "");
            } else if (step.capability_id == "browser.open_url") {
                step.expected_postcondition = "browser.navigated";
            } else if (step.capability_id == "media.volume") {
                step.expected_postcondition = "volume.set";
            }
        }

        // Dependencies extraction
        size_t dep_idx = block.find("\"depends_on\"");
        if (dep_idx == std::string::npos) dep_idx = block.find("\"dependencies\"");
        if (dep_idx != std::string::npos) {
            size_t d_start = block.find("[", dep_idx);
            size_t d_end = block.find("]", d_start);
            if (d_start != std::string::npos && d_end != std::string::npos) {
                std::string dep_block = block.substr(d_start + 1, d_end - d_start - 1);
                std::regex d_re("\"([^\"]+)\"");
                auto d_begin = std::sregex_iterator(dep_block.begin(), dep_block.end(), d_re);
                auto d_end_it = std::sregex_iterator();
                for (auto it = d_begin; it != d_end_it; ++it) {
                    step.dependencies.push_back((*it)[1].str());
                    plan.dependencies.push_back({(*it)[1].str(), step.step_id});
                }
            }
        }

        step.timeout_ms = 15000;
        step.retry_policy_max_attempts = 2;
        step.risk_level = (step.capability_id == "terminal.execute") ? contracts::RiskLevel::High : contracts::RiskLevel::Low;

        plan.steps.push_back(std::move(step));
        step_num++;
    }

    if (plan.steps.empty()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::ValidationError,
            "Model JSON parsed into 0 valid AgentSteps"
        );
    }

    plan.expected_outcomes.push_back("Execution of " + std::to_string(plan.steps.size()) + " steps via Ollama LLM");
    return contracts::Result<contracts::AgentPlan>::success(plan);
}

contracts::Result<contracts::AgentPlan> OllamaAgentModelProvider::generate_plan(
    const contracts::AgentRequest& request,
    const ShortTermTaskMemoryPtr& memory
) {
    (void)memory;

    if (!ping()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::Unavailable,
            "AGENT_UNAVAILABLE_OFFLINE: Local Ollama service is not reachable on " + host_ + ":" + std::to_string(port_)
        );
    }

    std::ostringstream prompt_builder;
    prompt_builder << "You are the VANI Mark 2 Agent Planner. Generate a strict JSON execution plan for the user goal.\n"
                   << "Available Capabilities:\n"
                   << "- application.launch: Arguments: {\"app_name\": \"<app>\"}. Postcondition: \"process.running:<app>\"\n"
                   << "- application.close: Arguments: {\"app_name\": \"<app>\"}. Postcondition: \"process.terminated:<app>\"\n"
                   << "- filesystem.write: Arguments: {\"path\": \"<filepath>\", \"content\": \"<text>\"}. Postcondition: \"file.exists:<filepath>\"\n"
                   << "- filesystem.read: Arguments: {\"path\": \"<filepath>\"}. Postcondition: \"file.read:<filepath>\"\n"
                   << "- browser.open_url: Arguments: {\"url\": \"<url>\"}. Postcondition: \"browser.navigated\"\n"
                   << "- media.volume: Arguments: {\"level\": \"<number>\"}. Postcondition: \"volume.set\"\n\n"
                   << "User Goal: " << request.goal << "\n\n"
                   << "Output ONLY a valid JSON object matching this schema:\n"
                   << "{\n"
                   << "  \"goal\": \"" << request.goal << "\",\n"
                   << "  \"steps\": [\n"
                   << "    {\n"
                   << "      \"id\": \"step_1\",\n"
                   << "      \"capability\": \"application.launch\",\n"
                   << "      \"arguments\": {\"app_name\": \"notepad.exe\"},\n"
                   << "      \"depends_on\": [],\n"
                   << "      \"postcondition\": \"process.running:notepad.exe\"\n"
                   << "    }\n"
                   << "  ]\n"
                   << "}";

    auto call_res = call_generate_api(prompt_builder.str(), request.request_id);
    if (!call_res.is_ok()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            call_res.error().code,
            call_res.error().message
        );
    }

    return parse_plan_json(call_res.value(), request.request_id);
}

contracts::Result<contracts::AgentPlan> OllamaAgentModelProvider::replan(
    const contracts::AgentRequest& request,
    const contracts::AgentPlan& current_plan,
    const std::string& failed_step_id,
    const contracts::AgentObservation& failure_obs,
    const ShortTermTaskMemoryPtr& memory
) {
    (void)memory;

    if (!ping()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::Unavailable,
            "AGENT_UNAVAILABLE_OFFLINE: Local Ollama service is not reachable during replan."
        );
    }

    std::ostringstream prompt_builder;
    prompt_builder << "You are the VANI Mark 2 Agent Planner. A previously planned step failed in the environment.\n"
                   << "User Goal: " << request.goal << "\n"
                   << "Failed Step ID: " << failed_step_id << "\n"
                   << "Failure Reason: " << failure_obs.error << "\n\n"
                   << "CRITICAL RECOVERY DIRECTIVE:\n"
                   << "Step " << failed_step_id << " failed permanently in the OS. You MUST NOT repeat or include the failed step or failed application.\n"
                   << "Generate an alternative recovery plan (Plan B) using the fallback alternative to fulfill the user's objective.\n\n"
                   << "Available Capabilities:\n"
                   << "- application.launch: Arguments: {\"app_name\": \"<app>\"}\n"
                   << "- application.close: Arguments: {\"app_name\": \"<app>\"}\n"
                   << "- filesystem.write: Arguments: {\"path\": \"<filepath>\", \"content\": \"<text>\"}\n"
                   << "- filesystem.read: Arguments: {\"path\": \"<filepath>\"}\n"
                   << "- browser.open_url: Arguments: {\"url\": \"<url>\"}\n\n"
                   << "Generate a replacement recovery plan (Plan B) to achieve the goal with an alternate strategy.\n"
                   << "Output ONLY a valid JSON object matching this schema:\n"
                   << "{\n"
                   << "  \"goal\": \"" << request.goal << "\",\n"
                   << "  \"steps\": [\n"
                   << "    {\n"
                   << "      \"id\": \"step_1\",\n"
                   << "      \"capability\": \"application.launch\",\n"
                   << "      \"arguments\": {\"app_name\": \"notepad.exe\"},\n"
                   << "      \"depends_on\": [],\n"
                   << "      \"postcondition\": \"process.running:notepad.exe\"\n"
                   << "    }\n"
                   << "  ]\n"
                   << "}";

    auto call_res = call_generate_api(prompt_builder.str(), request.request_id + "_replan");
    if (!call_res.is_ok()) {
        return contracts::Result<contracts::AgentPlan>::failure(
            call_res.error().code,
            call_res.error().message
        );
    }

    auto plan_res = parse_plan_json(call_res.value(), request.request_id + "_replan");
    if (plan_res.is_ok()) {
        contracts::AgentPlan plan = plan_res.value();
        plan.plan_id = current_plan.plan_id + "_replan";
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }
    return plan_res;
}

contracts::Result<std::string> OllamaAgentModelProvider::summarize_observation(
    const contracts::AgentObservation& observation
) {
    if (observation.success) {
        return contracts::Result<std::string>::success("Step " + observation.step_id + " verified: " + observation.evidence);
    } else {
        return contracts::Result<std::string>::success("Step " + observation.step_id + " failed: " + observation.error);
    }
}

} // namespace vani::runtime::agent
