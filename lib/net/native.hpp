#ifndef BAULK_NET_NATIVE_HPP
#define BAULK_NET_NATIVE_HPP
#include <bela/env.hpp>
#include <bela/strip.hpp>
#include <baulk/net/types.hpp>
#include <schannel.h>
#include <ws2tcpip.h>
#include <winhttp.h>

struct WINHTTP_SECURITY_INFO_X {
  SecPkgContext_ConnectionInfo ConnectionInfo;
  SecPkgContext_CipherInfo CipherInfo;
};

#ifndef WINHTTP_OPTION_SECURITY_INFO
#define WINHTTP_OPTION_SECURITY_INFO 151
#endif

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#endif

#ifndef WINHTTP_PROTOCOL_FLAG_HTTP3
#define WINHTTP_PROTOCOL_FLAG_HTTP3 0x2
#endif

namespace baulk::net::native {
using baulk::net::net_internal::make_net_error_code;
struct url {
  std::wstring host;
  std::wstring filename;
  std::wstring uri;
  int nPort{80};
  int nScheme{INTERNET_SCHEME_HTTPS};
  inline DWORD TlsFlag() const { return nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0; }
};

inline std::optional<url> crack_url(std::wstring_view us, bela::error_code &ec) {
  URL_COMPONENTSW uc;
  ZeroMemory(&uc, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  uc.dwSchemeLength = (DWORD)-1;
  uc.dwHostNameLength = (DWORD)-1;
  uc.dwUrlPathLength = (DWORD)-1;
  uc.dwExtraInfoLength = (DWORD)-1;
  if (WinHttpCrackUrl(us.data(), static_cast<DWORD>(us.size()), 0, &uc) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  std::wstring_view urlpath{uc.lpszUrlPath, uc.dwUrlPathLength};
  return std::make_optional(
      url{.host = {uc.lpszHostName, uc.dwHostNameLength},
          .filename = url_path_name(urlpath),
          .uri = bela::StringCat(urlpath, std::wstring_view{uc.lpszExtraInfo, uc.dwExtraInfoLength}),
          .nPort = uc.nPort,
          .nScheme = uc.nScheme});
}

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
// https://www.rfc-editor.org/rfc/rfc6266#section-5
inline std::optional<std::wstring> extract_filename(const headers_t &hkv) {
  auto it = hkv.find(L"Content-Disposition");
  if (it == hkv.end()) {
    return std::nullopt;
  }
  std::vector<std::wstring_view> pvv = bela::StrSplit(it->second, bela::ByChar(';'), bela::SkipEmpty());
  constexpr std::wstring_view fns = L"filename=";
  constexpr std::wstring_view fnsu = L"filename*=";
  for (auto e : pvv) {
    auto s = bela::StripAsciiWhitespace(e);
    if (bela::ConsumePrefix(&s, fns)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      return std::make_optional<>(url_path_name(s));
    }
    if (bela::ConsumePrefix(&s, fnsu)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      return std::make_optional<>(url_decode(url_path_name(s)));
    }
  }
  return std::nullopt;
}

// content_length
inline int64_t content_length(const headers_t &hkv) {
  if (auto it = hkv.find(L"Content-Length"); it != hkv.end()) {
    if (int64_t len = 0; bela::SimpleAtoi(bela::StripAsciiWhitespace(it->second), &len)) {
      return len;
    }
  }
  return -1;
}

inline bool enable_part_download(const headers_t &hkv) {
  if (auto it = hkv.find(L"Accept-Ranges"); it != hkv.end()) {
    return bela::EqualsIgnoreCase(bela::StripAsciiWhitespace(it->second), L"bytes");
  }
  return false;
}

class handle {
public:
  handle() = default;
  handle(HINTERNET h_) : h(h_) {}
  handle(const handle &) = delete;
  handle &operator=(const handle &) = delete;
  ~handle() {
    if (h != nullptr) {
      WinHttpCloseHandle(h);
    }
  }
  auto addressof() const { return h; }
  bool set_proxy_url(std::wstring &url) {
    WINHTTP_PROXY_INFOW proxy;
    proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy.lpszProxy = url.data();
    proxy.lpszProxyBypass = nullptr;
    return WinHttpSetOption(h, WINHTTP_OPTION_PROXY, &proxy, sizeof(proxy)) == TRUE;
  }
  void protocol_enable() {
    // ENABLE TLS 1.3
    DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
    if (WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols)) != TRUE) {
      secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
      WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
    }
    // ENABLE HTTP2 and HTTP3
    DWORD all_protocols(WINHTTP_PROTOCOL_FLAG_HTTP2 | WINHTTP_PROTOCOL_FLAG_HTTP3);
    if (WinHttpSetOption(h, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &all_protocols, sizeof(all_protocols)) != TRUE) {
      all_protocols = WINHTTP_PROTOCOL_FLAG_HTTP2;
      WinHttpSetOption(h, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &all_protocols, sizeof(all_protocols));
    }
  }
  void set_insecure_mode() {
    // Ignore check tls
    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
    WinHttpSetOption(h, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
  }
  // fill header
  bool write_headers(const headers_t &hkv, const std::vector<std::wstring> &cookies, int64_t position, int64_t length,
                     bela::error_code &ec) {
    std::wstring flattened_headers;
    for (const auto &[key, value] : hkv) {
      bela::StrAppend(&flattened_headers, key, L": ", value, L"\r\n");
    }
    // part download
    if (position > 0) {
      bela::StrAppend(&flattened_headers, L"Range: bytes=", position, L"-", length - 1);
    }
    if (!cookies.empty()) {
      bela::StrAppend(&flattened_headers, L"Cookie: ", bela::StrJoin(cookies, L"; "), L"\r\n");
    }

    if (flattened_headers.empty()) {
      return true;
    }
    if (WinHttpAddRequestHeaders(h, flattened_headers.data(), static_cast<DWORD>(flattened_headers.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD) != TRUE) {
      ec = make_net_error_code();
      return false;
    }
    return true;
  }
  bool write_body(std::wstring_view body, std::wstring_view content_type, bela::error_code &ec) {
    if (body.empty()) {
      if (WinHttpSendRequest(h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != TRUE) {
        ec = make_net_error_code();
        return false;
      }
      return true;
    }
    auto addheader = bela::StringCat(L"Content-Type: ", content_type.empty() ? content_type : L"text/plain",
                                     L"\r\nContent-Length: ", body.size(), L"\r\n");
    if (WinHttpAddRequestHeaders(h, addheader.data(), static_cast<DWORD>(addheader.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) != TRUE) {
      ec = make_net_error_code();
      return false;
    }
    if (WinHttpSendRequest(h, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           const_cast<LPVOID>(reinterpret_cast<LPCVOID>(body.data())), static_cast<DWORD>(body.size()),
                           static_cast<DWORD>(body.size()), 0) != TRUE) {
      ec = make_net_error_code();
      return false;
    }
    return true;
  }
  // recv_minimal_response: read minimal response --> status code and headers
  std::optional<minimal_response> recv_minimal_response(bela::error_code &ec) {
    if (WinHttpReceiveResponse(h, nullptr) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    DWORD status_code = 0;
    DWORD dwSize = sizeof(status_code);
    if (WinHttpQueryHeaders(h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status_code, &dwSize,
                            nullptr) != TRUE) {
      ec = make_net_error_code(L"status code");
      return std::nullopt;
    }
    std::wstring status;
    status.resize(128);
    dwSize = static_cast<DWORD>(status.size() * 2);
    if (WinHttpQueryHeaders(h, WINHTTP_QUERY_STATUS_TEXT, nullptr, status.data(), &dwSize, nullptr) == TRUE) {
      status.resize(dwSize / 2);
    }
    dwSize = 0;
    WinHttpQueryHeaders(h, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                        &dwSize, WINHTTP_NO_HEADER_INDEX); // ignore error
    std::wstring buffer;
    buffer.resize(dwSize / 2 + 1);
    if (WinHttpQueryHeaders(h, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &dwSize,
                            WINHTTP_NO_HEADER_INDEX) != TRUE) {
      ec = make_net_error_code(L"raw headers");
      return std::nullopt;
    }
    buffer.resize(dwSize / 2);
    DWORD dwOption = 0;
    dwSize = sizeof(dwOption);
    auto version = protocol_version::HTTP11;
    if (WinHttpQueryOption(h, WINHTTP_OPTION_HTTP_PROTOCOL_USED, &dwOption, &dwSize) == TRUE) {
      if ((dwOption & WINHTTP_PROTOCOL_FLAG_HTTP3) != 0) {
        version = protocol_version::HTTP3;
      } else if ((dwOption & WINHTTP_PROTOCOL_FLAG_HTTP2) != 0) {
        version = protocol_version::HTTP2;
      }
    }
    // split headers
    std::vector<std::wstring_view> hlines = bela::StrSplit(buffer, bela::ByString(L"\r\n"), bela::SkipEmpty());
    headers_t hdrs;
    for (size_t i = 1; i < hlines.size(); i++) {
      auto line = hlines[i];
      if (auto pos = line.find(':'); pos != std::wstring_view::npos) {
        auto k = bela::StripAsciiWhitespace(line.substr(0, pos));
        auto v = bela::StripAsciiWhitespace(line.substr(pos + 1));
        hdrs.emplace(k, v);
      }
    }
    return std::make_optional(minimal_response{.headers = std::move(hdrs),
                                               .status_code = static_cast<unsigned long>(status_code),
                                               .version = version,
                                               .status_text = std::move(status)});
  }
  //
  int64_t recv_completely(int64_t len, std::vector<char> &buffer, size_t max_body_size, bela::error_code &ec) {
    if (len == 0) {
      return 0;
    }
    // content-length
    if (len > 0) {
      auto total_size =
          static_cast<size_t>((std::min)(static_cast<uint64_t>(max_body_size), static_cast<uint64_t>(len)));
      buffer.resize(total_size);
      size_t download_size = 0;
      auto buf = buffer.data();
      do {
        DWORD dwSize = 0;
        if (WinHttpQueryDataAvailable(h, &dwSize) != TRUE) {
          ec = make_net_error_code();
          return -1;
        }
        auto recv_size = (std::min)(total_size - download_size, static_cast<size_t>(dwSize));
        if (WinHttpReadData(h, buf + download_size, static_cast<DWORD>(recv_size), &dwSize) != TRUE) {
          ec = make_net_error_code();
          return -1;
        }
        download_size += static_cast<size_t>(dwSize);
      } while (total_size > download_size);
      return download_size;
    }
    // chunk receive
    buffer.resize(256 * 1024); // 256kb buffer
    size_t download_size = 0;
    do {
      DWORD dwSize = 0;
      if (WinHttpQueryDataAvailable(h, &dwSize) != TRUE) {
        ec = make_net_error_code();
        return -1;
      }
      if (dwSize == 0) {
        break;
      }
      if (buffer.size() < static_cast<size_t>(dwSize) + download_size) {
        buffer.resize(download_size + static_cast<size_t>(dwSize));
      }
      if (WinHttpReadData(h, buffer.data() + download_size, dwSize, &dwSize) != TRUE) {
        ec = make_net_error_code();
        return -1;
      }
      download_size += dwSize;
    } while (download_size < max_body_size);
    return download_size;
  }

  // a session handle create a connection
  std::optional<handle> connect(std::wstring_view host, int port, bela::error_code &ec) {
    auto hConnect = WinHttpConnect(h, host.data(), port, 0);
    if (hConnect == nullptr) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    return std::make_optional<handle>(hConnect);
  }
  // a connection handle open a request
  std::optional<handle> open_request(std::wstring_view method, std::wstring_view uri, DWORD dwFlags,
                                     bela::error_code &ec) {
    auto hRequest = WinHttpOpenRequest(h, method.data(), uri.data(), nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
    if (hRequest == nullptr) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    return std::make_optional<handle>(hRequest);
  }

private:
  friend std::optional<handle> make_session(std::wstring_view ua, bela::error_code &ec);
  HINTERNET h{nullptr};
};

// make a session handle
inline std::optional<handle> make_session(std::wstring_view ua, bela::error_code &ec) {
  auto hSession =
      WinHttpOpen(ua.data(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (hSession == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  return std::make_optional<handle>(hSession);
}

} // namespace baulk::net::native

#endif