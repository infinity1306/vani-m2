#include "core/runtime.hpp"
#include "observability/logger.hpp"
#include <iostream>

int main() {
    vani::observability::Logger::instance().info(
        "vani-gateway",
        "VANI IPC / WebSocket Transport Gateway initialized on port 8765."
    );
    return 0;
}
