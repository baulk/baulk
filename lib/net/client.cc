//
#include <bela/env.hpp>
#include <baulk/net/client.hpp>
#include "lowlayer.hpp"

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
                                            std::wstring_view content_type, std::wstring_view body,
                                            bela::error_code &ec) {
  auto u = native::crack_url(url, ec);
  if (!u) {
    return std::nullopt;
  }
  auto session = native::make_session(userAgent, ec);
  if (!session) {
    return std::nullopt;
  }
  if (!IsNoProxy(u->host)) {
    session->set_proxy_url(proxyURL);
  }
  session->protocol_enable();
  auto conn = session->connect(u->host, u->nPort, ec);
  if (!conn) {
    return std::nullopt;
  }
  auto req = conn->open_request(method, u->uri, u->TlsFlag(), ec);
  if (!req) {
    return std::nullopt;
  }
  if (insecureMode) {
    req->set_insecure_mode();
  }
  if (!req->write_headers(hkv, cookies, ec)) {
    return std::nullopt;
  }
  if (!req->write_body(body, content_type, ec)) {
    return std::nullopt;
  }
  auto mr = req->recv_minimal_response(ec);
  if (!mr) {
    return std::nullopt;
  }
  std::vector<char> buffer;
  int64_t recv_size = -1;
  if (recv_size = req->recv_completely(buffer, direct_max_body_size, ec); recv_size < 0) {
    return std::nullopt;
  }
  return std::make_optional<Response>(std::move(mr), std::move(buffer), static_cast<size_t>(recv_size));
}
std::optional<std::wstring> HttpClient::WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                               bela::error_code &ec) {
  return std::nullopt;
}
} // namespace baulk::net