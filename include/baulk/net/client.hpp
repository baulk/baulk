#ifndef BAULK_NET_CLIENT_HPP
#define BAULK_NET_CLIENT_HPP
#include "types.hpp"
#include <filesystem>
#include <bela/terminal.hpp>

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
  // raw content bytes (usually UTF-8)
  std::string_view Content() const { return {body_.data(), size_}; }

private:
  std::vector<char> body_;
  size_t size_{0};
};

struct download_options {
  std::wstring hash_value;
  std::filesystem::path cwd;
  std::filesystem::path destination;
  bool force_overwrite{false};
  bool OverwriteExists() const { return force_overwrite || !destination.empty(); }
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
  std::optional<std::filesystem::path> WinGet(std::wstring_view url, const download_options &opts,
                                              bela::error_code &ec);

  std::wstring_view UserAgent() const { return userAgent; }
  std::wstring_view ProxyURL() const { return proxyURL; }
  bool IsInsecureMode() const { return insecureMode; }
  bool IsDebugMode() const { return debugMode; }
  bool IsNoCache() const { return noCache; }
  bool IsNoProxy(std::wstring_view host) const;
  void SetUserAgent(std::wstring_view ua) { userAgent = ua; }
  void SetMaxBodySize(int64_t size) { max_body_size = size; }
  void SetInsecureMode(bool m) { insecureMode = m; }
  void SetDebugMode(bool m) { debugMode = m; }
  void SetNoCache(bool n) { noCache = n; }
  void SetProxyURL(std::wstring_view url) { proxyURL = url; }
  bool InitializeProxyFromEnv();
  
  static HttpClient &DefaultClient() {
    static HttpClient client;
    return client;
  }

  template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, const Args &...args) {
    if (!debugMode) {
      return 0;
    }
    const bela::format_internal::FormatArg arg_array[] = {args...};
    std::wstring str;
    str.append(L"\x1b[33m* ");
    bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
    if (str.back() == '\n') {
      str.pop_back();
    }
    str.append(L"\x1b[0m\n");
    return bela::terminal::WriteAuto(stderr, str);
  }
  bela::ssize_t DbgPrint(const wchar_t *fmt) {
    if (!debugMode) {
      return 0;
    }
    std::wstring_view msg(fmt);
    if (!msg.empty() && msg.back() == '\n') {
      msg.remove_suffix(1);
    }
    return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
  }

private:
  headers_t hkv;
  std::wstring userAgent{L"Wget/7.0 (Baulk)"};
  std::wstring proxyURL;
  std::vector<std::wstring> cookies;
  std::vector<std::wstring> noProxy;
  size_t max_body_size{128 * 1024 * 1024};
  bool insecureMode{false};
  bool debugMode{false};
  bool noCache{false};
};

// HTTP rest api
inline std::optional<Response> RestGet(std::wstring_view url, bela::error_code &ec) {
  return HttpClient::DefaultClient().WinRest(L"GET", url, L"", L"", ec);
}

// WinGet download file
inline std::optional<std::filesystem::path> WinGet(std::wstring_view url, const download_options &opts,
                                                   bela::error_code &ec) {
  return HttpClient::DefaultClient().WinGet(url, opts, ec);
}

} // namespace baulk::net

#endif