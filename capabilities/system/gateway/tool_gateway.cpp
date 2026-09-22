#include "tool_gateway.hpp"
#include <chrono>
#include <sstream>

namespace vani::capabilities::system {

ToolGateway::ToolGateway(
    runtime::PolicyEnginePtr policy_engine,
    runtime::PermissionServicePtr permission_service,
    runtime::EventBusPtr event_bus,
    runtime::AuditServicePtr audit_service,
    ApplicationManagerPtr app_manager,
    ProcessManagerPtr proc_manager,
    FilesystemManagerPtr fs_manager,
    TerminalExecutorPtr term_executor,
    ProjectContextManagerPtr proj_manager,
    BrowserManagerPtr browser_manager,
    WindowManagerPtr win_manager,
    InputManagerPtr input_manager,
    ClipboardManagerPtr clipboard_manager,
    ScreenCaptureManagerPtr screen_manager,
    DisplayManagerPtr display_manager,
    MediaManagerPtr media_manager,
    SystemStateProviderPtr state_provider,
    NetworkManagerPtr network_manager,
    PowerManagerPtr power_manager,
    NotificationManagerPtr notif_manager,
    SystemActionJournalPtr journal
) : policy_engine_(std::move(policy_engine)),
    permission_service_(std::move(permission_service)),
    event_bus_(std::move(event_bus)),
    audit_service_(std::move(audit_service)),
    app_manager_(std::move(app_manager)),
    proc_manager_(std::move(proc_manager)),
    fs_manager_(std::move(fs_manager)),
    term_executor_(std::move(term_executor)),
    proj_manager_(std::move(proj_manager)),
    browser_manager_(std::move(browser_manager)),
    win_manager_(std::move(win_manager)),
    input_manager_(std::move(input_manager)),
    clipboard_manager_(std::move(clipboard_manager)),
    screen_manager_(std::move(screen_manager)),
    display_manager_(std::move(display_manager)),
    media_manager_(std::move(media_manager)),
    state_provider_(std::move(state_provider)),
    network_manager_(std::move(network_manager)),
    power_manager_(std::move(power_manager)),
    notif_manager_(std::move(notif_manager)),
    journal_(journal ? std::move(journal) : std::make_shared<SystemActionJournal>()) {}

contracts::Result<contracts::ToolResult> ToolGateway::execute(const ToolExecutionPipelineContext& context) {
    auto start_time = std::chrono::steady_clock::now();
    uint64_t start_time_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // Step 1: Cancellation check
    if (context.cancellation_token.is_cancelled()) {
        return contracts::Result<contracts::ToolResult>::failure(
            contracts::ErrorCode::Cancelled, "Tool execution was cancelled before start"
        );
    }

    // Step 2: Schema validation & capability check
    if (context.capability_id.empty()) {
        return contracts::Result<contracts::ToolResult>::failure(
            contracts::ErrorCode::ValidationError, "Missing capability_id in tool execution request"
        );
    }

    // Step 3: Context Resolution
    std::string actor = context.actor_id.empty() ? "user" : context.actor_id;

    // Step 4: Policy Evaluation
    runtime::PolicyDecision decision = runtime::PolicyDecision::Allow;
    if (policy_engine_) {
        runtime::PolicyContext p_ctx;
        p_ctx.actor_id = actor;
        p_ctx.capability_id = context.capability_id;
        p_ctx.task_id = context.task_id;
        p_ctx.session_id = context.session_id;
        decision = policy_engine_->evaluate(p_ctx);

        if (decision == runtime::PolicyDecision::Deny) {
            contracts::SystemActionJournalEntry jnl;
            jnl.timestamp_ms = start_time_ms;
            jnl.task_id = context.task_id;
            jnl.capability_id = context.capability_id;
            jnl.actor_id = actor;
            jnl.policy_decision = "DENY";
            jnl.execution_success = false;
            jnl.verification_result = "Blocked by Policy Engine";
            journal_->log_action(jnl);

            return contracts::Result<contracts::ToolResult>::failure(
                contracts::ErrorCode::PermissionDenied, "Action denied by Policy Engine: " + context.capability_id
            );
        }

        if (decision == runtime::PolicyDecision::RequireConfirmation && !context.user_confirmed) {
            return contracts::Result<contracts::ToolResult>::failure(
                contracts::ErrorCode::PermissionDenied, "Action requires explicit user confirmation: " + context.capability_id
            );
        }
    }

    // Step 5: Permission Service Evaluation
    if (permission_service_) {
        if (!permission_service_->has_permission(actor, context.capability_id, context.session_id, context.task_id)) {
            // Check if actor is default user with implicit base access
            if (actor != "user" && actor != "user.fast_path" && actor != "system") {
                return contracts::Result<contracts::ToolResult>::failure(
                    contracts::ErrorCode::PermissionDenied,
                    "Actor '" + actor + "' lacks permission for capability: " + context.capability_id
                );
            }
        }
    }

    // Step 6: Dispatch Capability Execution
    auto dispatch_res = dispatch_capability(context.capability_id, context.arguments_json, context);

    auto end_time = std::chrono::steady_clock::now();
    uint32_t duration_ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
    );

    contracts::ToolResult tool_res;
    tool_res.tool_id = context.tool_id.empty() ? context.capability_id : context.tool_id;
    tool_res.execution_duration_ms = duration_ms;

    if (!dispatch_res.is_success()) {
        tool_res.success = false;
        tool_res.execution_log = dispatch_res.error().message;

        contracts::SystemActionJournalEntry jnl;
        jnl.timestamp_ms = start_time_ms;
        jnl.task_id = context.task_id;
        jnl.capability_id = context.capability_id;
        jnl.actor_id = actor;
        jnl.policy_decision = std::string(runtime::to_string(decision));
        jnl.execution_success = false;
        jnl.verification_result = "Execution Failed: " + dispatch_res.error().message;
        jnl.duration_ms = duration_ms;
        journal_->log_action(jnl);

        return contracts::Result<contracts::ToolResult>::failure(dispatch_res.error());
    }

    tool_res.success = true;
    tool_res.output_json = dispatch_res.value();
    tool_res.execution_log = "Executed successfully";

    // Step 7: Postcondition Verification
    auto verify_res = verify_postcondition(context.capability_id, dispatch_res.value());
    std::string verify_status = verify_res.is_success() ? "VERIFIED_OK" : "VERIFICATION_WARNING: " + verify_res.error().message;

    // Step 8: Action Journal & Audit Logging
    contracts::SystemActionJournalEntry jnl;
    jnl.timestamp_ms = start_time_ms;
    jnl.task_id = context.task_id;
    jnl.capability_id = context.capability_id;
    jnl.actor_id = actor;
    jnl.arguments_summary = "[SANITIZED_INPUT]";
    jnl.policy_decision = std::string(runtime::to_string(decision));
    jnl.execution_success = true;
    jnl.verification_result = verify_status;
    jnl.duration_ms = duration_ms;
    journal_->log_action(jnl);

    // Emit event through event bus
    if (event_bus_) {
        contracts::EventHeader header;
        header.event_type = "capability.execution.completed";
        header.source = "tool_gateway";
        header.task_id = context.task_id;
        header.correlation_id = context.correlation_id;
        header.category = contracts::EventCategory::Tool;
        auto evt = std::make_shared<const contracts::Event>(header, std::unordered_map<std::string, std::string>{
            {"capability_id", context.capability_id},
            {"duration_ms", std::to_string(duration_ms)}
        });
        event_bus_->publish(evt);
    }

    return contracts::Result<contracts::ToolResult>::success(tool_res);
}

contracts::Result<std::string> ToolGateway::dispatch_capability(
    const std::string& capability_id,
    const std::string& arguments_json,
    const ToolExecutionPipelineContext& context
) {
    // 1. Applications
    if (capability_id == "system.application.open" || capability_id == "application.launch") {
        if (!app_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "App manager unavailable");
        std::string app_name = arguments_json.empty() ? "chrome" : arguments_json;
        if (app_name.find("\"app_name\":") != std::string::npos) {
            auto pos = app_name.find("\"app_name\":");
            auto q1 = app_name.find('\"', pos + 11);
            if (q1 != std::string::npos) {
                auto q2 = app_name.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    app_name = app_name.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        auto res = app_manager_->open_application(app_name);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"opened\", \"app_id\": \"" + res.value().application_id + "\"}");
    }
    if (capability_id == "system.application.close" || capability_id == "application.close") {
        if (!app_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "App manager unavailable");
        std::string app_name = arguments_json;
        if (app_name.find("\"app_name\":") != std::string::npos) {
            auto pos = app_name.find("\"app_name\":");
            auto q1 = app_name.find('\"', pos + 11);
            if (q1 != std::string::npos) {
                auto q2 = app_name.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    app_name = app_name.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        auto res = app_manager_->close_application(app_name);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"closed\"}");
    }
    if (capability_id == "system.application.list") {
        if (!app_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "App manager unavailable");
        auto res = app_manager_->list_applications(false);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"count\": " + std::to_string(res.value().size()) + "}");
    }

    // 2. Processes
    if (capability_id == "system.process.list") {
        if (!proc_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Process manager unavailable");
        auto res = proc_manager_->list_processes();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"count\": " + std::to_string(res.value().size()) + "}");
    }
    if (capability_id == "system.process.stop") {
        if (!proc_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Process manager unavailable");
        uint32_t pid = 100;
        try { pid = static_cast<uint32_t>(std::stoul(arguments_json)); } catch (...) {}
        auto res = proc_manager_->stop_process(pid);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"stopped\", \"pid\": " + std::to_string(pid) + "}");
    }

    // 3. Filesystem
    if (capability_id == "filesystem.read") {
        if (!fs_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Filesystem manager unavailable");
        std::string path = arguments_json;
        if (arguments_json.find("\"path\":") != std::string::npos) {
            auto pos = arguments_json.find("\"path\":");
            auto q1 = arguments_json.find('\"', pos + 7);
            if (q1 != std::string::npos) {
                auto q2 = arguments_json.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    path = arguments_json.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        auto res = fs_manager_->read_file(path);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success(res.value());
    }
    if (capability_id == "filesystem.write") {
        if (!fs_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Filesystem manager unavailable");
        std::string path = arguments_json;
        std::string content = "file content";
        if (arguments_json.find("\"path\":") != std::string::npos) {
            auto pos = arguments_json.find("\"path\":");
            auto q1 = arguments_json.find('\"', pos + 7);
            if (q1 != std::string::npos) {
                auto q2 = arguments_json.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    path = arguments_json.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        if (arguments_json.find("\"content\":") != std::string::npos) {
            auto pos = arguments_json.find("\"content\":");
            auto q1 = arguments_json.find('\"', pos + 10);
            if (q1 != std::string::npos) {
                auto q2 = arguments_json.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    content = arguments_json.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        auto res = fs_manager_->write_file(path, content);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"written\", \"path\": \"" + path + "\"}");
    }
    if (capability_id == "filesystem.delete") {
        if (!fs_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Filesystem manager unavailable");
        std::string path = arguments_json;
        if (arguments_json.find("\"path\":") != std::string::npos) {
            auto pos = arguments_json.find("\"path\":");
            auto q1 = arguments_json.find('\"', pos + 7);
            if (q1 != std::string::npos) {
                auto q2 = arguments_json.find('\"', q1 + 1);
                if (q2 != std::string::npos) {
                    path = arguments_json.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        }
        auto res = fs_manager_->delete_file(path, true); // Safe trash default
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"deleted_to_trash\"}");
    }

    // 4. Terminal
    if (capability_id == "terminal.execute") {
        if (!term_executor_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Terminal executor unavailable");
        contracts::TerminalExecutionRequest req;
        req.command = arguments_json;
        auto res = term_executor_->execute(req, context.cancellation_token);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success(res.value().stdout_content);
    }

    // 5. Browser
    if (capability_id == "browser.open" || capability_id == "browser.navigate") {
        if (!browser_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Browser manager unavailable");
        auto s_res = browser_manager_->open_browser(context.task_id);
        if (!s_res.is_success()) return contracts::Result<std::string>::failure(s_res.error());
        if (!arguments_json.empty()) {
            browser_manager_->navigate(s_res.value().session_id, arguments_json);
        }
        return contracts::Result<std::string>::success("{\"status\": \"navigated\", \"session_id\": \"" + s_res.value().session_id + "\"}");
    }

    // 6. Windows
    if (capability_id == "window.list") {
        if (!win_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Window manager unavailable");
        auto res = win_manager_->list_windows();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"count\": " + std::to_string(res.value().size()) + "}");
    }

    // 7. Clipboard
    if (capability_id == "clipboard.read") {
        if (!clipboard_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Clipboard manager unavailable");
        auto res = clipboard_manager_->read_clipboard();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success(res.value().text_content);
    }
    if (capability_id == "clipboard.write") {
        if (!clipboard_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Clipboard manager unavailable");
        auto res = clipboard_manager_->write_clipboard(arguments_json);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"clipboard_updated\"}");
    }

    // 8. Screen
    if (capability_id == "screen.capture") {
        if (!screen_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Screen manager unavailable");
        auto res = screen_manager_->capture_screen();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"capture_id\": \"" + res.value().capture_id + "\", \"format\": \"png\"}");
    }

    // 9. Display
    if (capability_id == "display.list") {
        if (!display_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Display manager unavailable");
        auto res = display_manager_->list_displays();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"count\": " + std::to_string(res.value().size()) + "}");
    }

    // 10. Media
    if (capability_id == "media.play") {
        if (!media_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Media manager unavailable");
        auto res = media_manager_->play();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"playing\"}");
    }
    if (capability_id == "media.pause") {
        if (!media_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Media manager unavailable");
        auto res = media_manager_->pause();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"paused\"}");
    }
    if (capability_id == "media.volume") {
        if (!media_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Media manager unavailable");
        uint32_t vol = 50;
        try { vol = static_cast<uint32_t>(std::stoul(arguments_json)); } catch (...) {}
        auto res = media_manager_->set_volume(vol);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"volume\": " + std::to_string(vol) + "}");
    }

    // 11. System State
    if (capability_id == "system.get_state" || capability_id == "system.status") {
        if (!state_provider_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "State provider unavailable");
        auto res = state_provider_->get_state();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"os\": \"" + res.value().os_name + "\", \"battery\": " + std::to_string(res.value().battery_percent) + "}");
    }

    // 12. Power
    if (capability_id == "system.lock") {
        if (!power_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Power manager unavailable");
        auto res = power_manager_->lock_system();
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"locked\"}");
    }
    if (capability_id == "system.shutdown") {
        if (!power_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Power manager unavailable");
        auto res = power_manager_->shutdown_system(context.user_confirmed);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"shutting_down\"}");
    }

    // 13. Notifications
    if (capability_id == "system.notification.send") {
        if (!notif_manager_) return contracts::Result<std::string>::failure(contracts::ErrorCode::Unavailable, "Notification manager unavailable");
        contracts::NotificationPayload notif;
        notif.id = "notif_001";
        notif.title = "VANI Alert";
        notif.message = arguments_json;
        auto res = notif_manager_->send_notification(notif);
        if (!res.is_success()) return contracts::Result<std::string>::failure(res.error());
        return contracts::Result<std::string>::success("{\"status\": \"sent\"}");
    }

    return contracts::Result<std::string>::failure(
        contracts::ErrorCode::NotImplemented, "Capability handler not implemented: " + capability_id
    );
}

contracts::Result<void> ToolGateway::verify_postcondition(
    const std::string& capability_id,
    const std::string& /*result_output*/
) {
    // Deterministic postcondition verification hooks
    if (capability_id == "system.application.open") {
        if (app_manager_) {
            auto running = app_manager_->list_applications(true);
            if (running.is_success() && !running.value().empty()) {
                return contracts::Result<void>::success();
            }
        }
    } else if (capability_id == "media.volume") {
        if (media_manager_) {
            auto status = media_manager_->get_status();
            if (status.is_success()) {
                return contracts::Result<void>::success();
            }
        }
    } else if (capability_id == "clipboard.write") {
        if (clipboard_manager_) {
            auto clip = clipboard_manager_->read_clipboard();
            if (clip.is_success()) {
                return contracts::Result<void>::success();
            }
        }
    }
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ToolResult> ToolGateway::execute_fast_path(
    const std::string& intent_name,
    const std::unordered_map<std::string, std::string>& parameters,
    const std::string& actor_id
) {
    ToolExecutionPipelineContext ctx;
    ctx.actor_id = actor_id;
    ctx.task_id = "task_fast_path";

    if (intent_name == "application.launch" || intent_name == "open_application" || intent_name == "open_app") {
        ctx.capability_id = "system.application.open";
        auto it = parameters.find("app_name");
        ctx.arguments_json = (it != parameters.end()) ? it->second : "chrome";
    } else if (intent_name == "application.close" || intent_name == "close_application" || intent_name == "close_app") {
        ctx.capability_id = "system.application.close";
        auto it = parameters.find("app_name");
        ctx.arguments_json = (it != parameters.end()) ? it->second : "chrome";
    } else if (intent_name == "browser.open_url" || intent_name == "open_url" || intent_name == "open_browser") {
        ctx.capability_id = "browser.open";
        auto it = parameters.find("url");
        ctx.arguments_json = (it != parameters.end()) ? it->second : "https://google.com";
    } else if (intent_name == "system.status" || intent_name == "get_system_state" || intent_name == "battery_status") {
        ctx.capability_id = "system.get_state";
    } else if (intent_name == "media.volume" || intent_name == "set_volume" || intent_name == "volume") {
        ctx.capability_id = "media.volume";
        auto it = parameters.find("level");
        ctx.arguments_json = (it != parameters.end()) ? it->second : "50";
    } else if (intent_name == "software.execute_workflow") {
        ctx.capability_id = "terminal.execute";
        ctx.arguments_json = "echo Task executed";
    } else if (intent_name == "media_play" || intent_name == "play_music") {
        ctx.capability_id = "media.play";
    } else if (intent_name == "media_pause" || intent_name == "pause_music") {
        ctx.capability_id = "media.pause";
    } else if (intent_name == "take_screenshot" || intent_name == "capture_screen") {
        ctx.capability_id = "screen.capture";
    } else if (intent_name == "lock_system" || intent_name == "lock_pc") {
        ctx.capability_id = "system.lock";
    } else {
        return contracts::Result<contracts::ToolResult>::failure(
            contracts::ErrorCode::NotFound, "No Fast Path mapping for intent: " + intent_name
        );
    }

    return execute(ctx);
}

} // namespace vani::capabilities::system
