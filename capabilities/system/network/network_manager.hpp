#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>

namespace vani::capabilities::system {

struct NetworkState {
    bool is_online{true};
    std::string connection_type{"Wi-Fi"};
    std::string interface_name{"wlan0"};
    uint32_t latency_ms{12};
    std::string ip_address{"192.168.1.100"};
};

class NetworkManager {
public:
    explicit NetworkManager(adapters::system::SystemAdapterPtr adapter);
    ~NetworkManager() = default;

    contracts::Result<NetworkState> get_network_state();
    contracts::Result<void> modify_network_state(const std::string& config); // Highly guarded

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using NetworkManagerPtr = std::shared_ptr<NetworkManager>;

} // namespace vani::capabilities::system
