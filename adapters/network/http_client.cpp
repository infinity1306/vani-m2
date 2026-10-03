#include "http_client.hpp"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

#include <iostream>

namespace vani::adapters::network {

#ifdef _WIN32
static std::wstring to_wide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

struct ParsedUrl {
    bool is_https{false};
    std::wstring host;
    INTERNET_PORT port{80};
    std::wstring path;
};

static bool parse_url(const std::string& url_str, ParsedUrl& out) {
    std::wstring w_url = to_wide(url_str);
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = static_cast<DWORD>(-1);
    urlComp.dwHostNameLength = static_cast<DWORD>(-1);
    urlComp.dwUrlPathLength = static_cast<DWORD>(-1);
    urlComp.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(w_url.c_str(), static_cast<DWORD>(w_url.length()), 0, &urlComp)) {
        return false;
    }

    out.is_https = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
    out.host = std::wstring(urlComp.lpszHostName, urlComp.dwHostNameLength);
    out.port = urlComp.nPort;
    
    std::wstring path = (urlComp.dwUrlPathLength > 0) ? std::wstring(urlComp.lpszUrlPath, urlComp.dwUrlPathLength) : L"/";
    if (urlComp.dwExtraInfoLength > 0) {
        path += std::wstring(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
    }
    out.path = path;
    return true;
}
#endif

contracts::Result<HttpResponse> HttpClient::get(const std::string& url, uint32_t timeout_ms) {
#ifdef _WIN32
    ParsedUrl purl;
    if (!parse_url(url, purl)) {
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::ValidationError, "Invalid URL format: " + url
        );
    }

    HINTERNET hSession = WinHttpOpen(
        L"VANI-Desktop-Assistant/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::InternalError, "Failed to initialize WinHttp session"
        );
    }

    WinHttpSetTimeouts(hSession, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    HINTERNET hConnect = WinHttpConnect(hSession, purl.host.c_str(), purl.port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to connect to host"
        );
    }

    DWORD flags = purl.is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        purl.path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to create HTTP request"
        );
    }

    BOOL bSend = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!bSend || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to send or receive HTTP response"
        );
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &dwStatusCode,
        &dwSize,
        WINHTTP_NO_HEADER_INDEX
    );

    HttpResponse resp;
    resp.status_code = static_cast<int>(dwStatusCode);

    std::string response_data;
    DWORD dwDownloaded = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;

        std::vector<char> buffer(dwSize);
        if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
            response_data.append(buffer.data(), dwDownloaded);
        }
    } while (dwSize > 0);

    resp.body = std::move(response_data);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return contracts::Result<HttpResponse>::success(resp);
#else
    (void)url; (void)timeout_ms;
    return contracts::Result<HttpResponse>::failure(
        contracts::ErrorCode::NotImplemented, "HttpClient not implemented for non-Windows"
    );
#endif
}

contracts::Result<HttpResponse> HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::string& content_type,
    uint32_t timeout_ms
) {
#ifdef _WIN32
    ParsedUrl purl;
    if (!parse_url(url, purl)) {
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::ValidationError, "Invalid URL format: " + url
        );
    }

    HINTERNET hSession = WinHttpOpen(
        L"VANI-Desktop-Assistant/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::InternalError, "Failed to initialize WinHttp session"
        );
    }

    WinHttpSetTimeouts(hSession, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    HINTERNET hConnect = WinHttpConnect(hSession, purl.host.c_str(), purl.port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to connect to host"
        );
    }

    DWORD flags = purl.is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        purl.path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to create HTTP request"
        );
    }

    std::wstring headers = L"Content-Type: " + to_wide(content_type) + L"\r\n";
    BOOL bSend = WinHttpSendRequest(
        hRequest,
        headers.c_str(),
        static_cast<DWORD>(headers.length()),
        const_cast<char*>(body.data()),
        static_cast<DWORD>(body.length()),
        static_cast<DWORD>(body.length()),
        0
    );

    if (!bSend || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return contracts::Result<HttpResponse>::failure(
            contracts::ErrorCode::NetworkError, "Failed to send or receive HTTP response"
        );
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &dwStatusCode,
        &dwSize,
        WINHTTP_NO_HEADER_INDEX
    );

    HttpResponse resp;
    resp.status_code = static_cast<int>(dwStatusCode);

    std::string response_data;
    DWORD dwDownloaded = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;

        std::vector<char> buffer(dwSize);
        if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
            response_data.append(buffer.data(), dwDownloaded);
        }
    } while (dwSize > 0);

    resp.body = std::move(response_data);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return contracts::Result<HttpResponse>::success(resp);
#else
    (void)url; (void)body; (void)content_type; (void)timeout_ms;
    return contracts::Result<HttpResponse>::failure(
        contracts::ErrorCode::NotImplemented, "HttpClient not implemented for non-Windows"
    );
#endif
}

} // namespace vani::adapters::network
