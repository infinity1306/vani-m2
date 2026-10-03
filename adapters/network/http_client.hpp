#pragma once

#include "../../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace vani::adapters::network {

struct HttpResponse {
    int status_code{0};
    std::string body;
    std::string content_type;
};

class HttpClient {
public:
    static contracts::Result<HttpResponse> get(const std::string& url, uint32_t timeout_ms = 5000);
    static contracts::Result<HttpResponse> post(
        const std::string& url,
        const std::string& body,
        const std::string& content_type = "application/json",
        uint32_t timeout_ms = 5000
    );
};

} // namespace vani::adapters::network
