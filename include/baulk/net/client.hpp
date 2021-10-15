#ifndef BAULK_NET_CLIENT_HPP
#define BAULK_NET_CLIENT_HPP
#include "types.hpp"

namespace baulk::net {

enum class Protocol { HTTP11, HTTP20, HTTP30 };
struct Response {
  net_internal::headers_t hkv;
  std::string body;
  long statuscode{0};
  Protocol protocol{Protocol::HTTP11};
  [[nodiscard]] bool IsSuccessStatusCode() const { return statuscode >= 200 && statuscode <= 299; }
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
  std::optional<Response> WinRest(std::wstring_view method, std::wstring_view url, std::wstring_view contenttype,
                                  std::wstring_view body, bela::error_code &ec);
  std::optional<Response> Get(std::wstring_view url, bela::error_code &ec) {
    return WinRest(L"GET", url, L"", L"", ec);
  }
  std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                     bela::error_code &ec);
  //
  std::wstring &UserAgent() { return userAgent; }
  std::wstring_view UserAgent() const { return userAgent; }
  bool IsInsecureMode() const { return insecureMode; }
  bool &InsecureMode() { return insecureMode; }
  bool IsTlsVersion13() const { return tlsVersion13; }
  bool &TlsVersion13() { return tlsVersion13; }
  bool IsRangeAccepted() const { return rangeAccepted; }
  bool &RangeAccepted() { return rangeAccepted; }
  bool InitializeProxyFromEnv();
  std::wstring &ProxyURL() { return proxyURL; }
  std::wstring_view ProxyURL() const { return proxyURL; }
  bool IsNoProxy(std::wstring_view host) const;
  static HttpClient &DefaultClient() {
    static HttpClient client;
    return client;
  }

private:
  net_internal::headers_t hkv;
  std::vector<std::wstring> cookies;
  std::wstring userAgent{L"Wget/7.0 (Baulk)"};
  std::vector<std::wstring> noProxy;
  std::wstring proxyURL;
  bool insecureMode{false};
  bool tlsVersion13{false};
  bool rangeAccepted{false};
};

inline std::optional<Response> RestGet(std::wstring_view url, bela::error_code &ec) {
  return HttpClient::DefaultClient().WinRest(L"GET", url, L"", L"", ec);
}
// Download to file
inline std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                          bela::error_code &ec) {
  HttpClient::DefaultClient().WinGet(url, workdir, forceoverwrite, ec);
}

} // namespace baulk::net

#endif