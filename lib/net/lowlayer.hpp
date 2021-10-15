#ifndef BAULK_NET_LOWLAYER_HPP
#define BAULK_NET_LOWLAYER_HPP
#include <bela/env.hpp>
#include <baulk/net/types.hpp>
#include <winhttp.h>

namespace baulk::net::native {
class handle {
public:
  handle() = default;
  handle(const handle &) = delete;
  handle &operator=(const handle &) = delete;
  ~handle() {
    if (h != nullptr) {
      WinHttpCloseHandle(h);
    }
  }
  auto addressof() { return &h; }
  HINTERNET operator()() const { return h; }
  int64_t content_length() const {
    wchar_t conlen[32];
    DWORD dwXsize = sizeof(conlen);
    int64_t len = -1;
    if (WinHttpQueryHeaders(h, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, conlen, &dwXsize,
                            WINHTTP_NO_HEADER_INDEX) == TRUE &&
        bela::SimpleAtoi(bela::StripAsciiWhitespace({conlen, dwXsize / 2}), &len)) {
      return len;
    }
    return -1;
  }
  bool set_proxy_url(std::wstring &url) {
    WINHTTP_PROXY_INFOW proxy;
    proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy.lpszProxy = url.data();
    proxy.lpszProxyBypass = nullptr;
    return WinHttpSetOption(h, WINHTTP_OPTION_PROXY, &proxy, sizeof(proxy)) == TRUE;
  }
  void turn_on_tls() {
    // try open tls 1.3
    DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
    if (WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols)) != TRUE) {
      secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
      WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
    }
    // Enable HTTP2
    DWORD dwFlags = WINHTTP_PROTOCOL_FLAG_HTTP2;
    DWORD dwSize = sizeof(dwFlags);
    WinHttpSetOption(h, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &dwFlags, sizeof(dwFlags));
  }
  void insecure_mode() {
    // Ignore check tls
    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
    WinHttpSetOption(h, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
  }

private:
  HINTERNET h{nullptr};
};
} // namespace baulk::net::native

#endif