//
#include <bela/env.hpp>
#include <baulk/net/client.hpp>

namespace baulk::net {
using baulk::net::net_internal::make_net_error_code;
bool HttpClient::IsNoProxy(std::wstring_view host) const {
  for (const auto &u : noProxy) {
    if (bela::EqualsIgnoreCase(u, host)) {
      return true;
    }
  }
  return false;
}

bool HttpClient::InitializeProxyFromEnv() {
  if (auto noProxyURL = bela::GetEnv(L"NO_PROXY"); noProxyURL.empty()) {
    noProxy = bela::StrSplit(noProxyURL, bela::ByChar(L','), bela::SkipEmpty());
  }
  if (proxyURL = bela::GetEnv(L"HTTPS_PROXY"); !proxyURL.empty()) {
    return true;
  }
  if (proxyURL = bela::GetEnv(L"HTTP_PROXY"); !proxyURL.empty()) {
    return true;
  }
  if (proxyURL = bela::GetEnv(L"ALL_PROXY"); !proxyURL.empty()) {
    return true;
  }
  return true;
}

std::optional<Response> HttpClient::WinRest(std::wstring_view method, std::wstring_view url,
                                            std::wstring_view contenttype, std::wstring_view body,
                                            bela::error_code &ec) {
  //
  return std::nullopt;
}
std::optional<std::wstring> HttpClient::WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                               bela::error_code &ec) {
  //
  return std::nullopt;
}
} // namespace baulk::net