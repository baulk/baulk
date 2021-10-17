#ifndef BAULK_NET_CLIENT_HPP
#define BAULK_NET_CLIENT_HPP
#include "types.hpp"

namespace baulk::net {
class Response : private minimal_response {
public:
  Response(minimal_response &&mr, std::vector<char> &&b, size_t sz) {
    version = mr.version;
    headers = std::move(mr.headers);
    status_code = mr.status_code;
    status_text = std::move(mr.status_text);
    body_ = std::move(b);
    size_ = sz;
  }
  auto &Headers() const { return headers; }
  auto StatusCode() const { return status_code; }
  std::wstring_view StatusLine() const { return status_text; }
  auto Version() const { return version; }
  std::string_view Content() const { return {body_.data(), size_}; }

private:
  std::vector<char> body_;
  size_t size_{0};
};

class HttpClient {
public:
  HttpClient() = default;
  HttpClient(const HttpClient &) = delete;
  HttpClient &operator=(const HttpClient &) = delete;
  HttpClient &Set(std::wstring_view key, std::wstring_view value) {
    hkv[key] = value;
    return *this;
  }
  HttpClient &AddCookie(std::wstring_view cookie) {
    cookies.emplace_back(cookie);
    return *this;
  }
  std::optional<Response> WinRest(std::wstring_view method, std::wstring_view url, std::wstring_view content_type,
                                  std::wstring_view body, bela::error_code &ec);
  std::optional<Response> Get(std::wstring_view url, bela::error_code &ec) {
    return WinRest(L"GET", url, L"", L"", ec);
  }
  std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view cwd, std::wstring_view hash_value,
                                     bool force_overwrite, bela::error_code &ec);
  //
  std::wstring &UserAgent() { return userAgent; }
  std::wstring_view UserAgent() const { return userAgent; }
  bool IsInsecureMode() const { return insecureMode; }
  bool &InsecureMode() { return insecureMode; }
  bool IsDebugMode() { return debugMode; }
  bool &DebugMode() { return debugMode; }
  size_t DirectMaxBodySize() const { return direct_max_body_size; }
  size_t &DirectMaxBodySize() { return direct_max_body_size; }
  bool InitializeProxyFromEnv();
  std::wstring &ProxyURL() { return proxyURL; }
  std::wstring_view ProxyURL() const { return proxyURL; }
  bool IsNoProxy(std::wstring_view host) const;
  static HttpClient &DefaultClient() {
    static HttpClient client;
    return client;
  }

private:
  headers_t hkv;
  std::vector<std::wstring> cookies;
  std::wstring userAgent{L"Wget/7.0 (Baulk)"};
  std::vector<std::wstring> noProxy;
  std::wstring proxyURL;
  size_t direct_max_body_size{128 * 1024 * 1024};
  bool insecureMode{false};
  bool debugMode{false};
};

// HTTP rest api
inline std::optional<Response> RestGet(std::wstring_view url, bela::error_code &ec) {
  return HttpClient::DefaultClient().WinRest(L"GET", url, L"", L"", ec);
}

// WinGet download file
inline std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view cwd, std::wstring_view hash_value,
                                          bool force_overwrite, bela::error_code &ec) {
  HttpClient::DefaultClient().WinGet(url, cwd, hash_value, force_overwrite, ec);
}

} // namespace baulk::net

#endif