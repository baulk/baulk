//
#include <bela/env.hpp>
#include <baulk/net/client.hpp>
#include <baulk/indicators.hpp>
#include "native.hpp"
#include "file.hpp"

namespace baulk::net {

inline std::optional<std::wstring> query_remote_address(HINTERNET hRequest) {
  WINHTTP_CONNECTION_INFO coninfo;
  DWORD dwSize = sizeof(WINHTTP_CONNECTION_INFO);
  if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_CONNECTION_INFO, &coninfo, &dwSize) != TRUE) {
    return std::nullopt;
  }
  wchar_t addrw[256];
  if (coninfo.RemoteAddress.ss_family == AF_INET) {
    auto addr = reinterpret_cast<const struct sockaddr_in *>(&coninfo.RemoteAddress);
    if (InetNtopW(AF_INET, &addr->sin_addr, addrw, sizeof(addrw) * 2) == nullptr) {
      return std::nullopt;
    }
    return std::make_optional<std::wstring>(addrw);
  }
  if (coninfo.RemoteAddress.ss_family == AF_INET6) {
    auto addr = reinterpret_cast<const struct sockaddr_in6 *>(&coninfo.RemoteAddress);
    if (InetNtopW(AF_INET6, &addr->sin6_addr, addrw, sizeof(addrw) * 2) == nullptr) {
      return std::nullopt;
    }
    return std::make_optional<std::wstring>(addrw);
  }
  return std::nullopt;
}

void connect_trace(HINTERNET hRequest) {
  WINHTTP_SECURITY_INFO_X si;
  DWORD dwSize = sizeof(si);
  if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_SECURITY_INFO, &si, &dwSize) != TRUE) {
    return;
  }
  if ((si.ConnectionInfo.dwProtocol & SP_PROT_TLS1_2_CLIENT) != 0) {
    bela::FPrintF(stderr, L"\x1b[33m* SSL connection using TLSv1.2 / %s\x1b[0m\n", si.CipherInfo.szCipherSuite);
  }
  if ((si.ConnectionInfo.dwProtocol & SP_PROT_TLS1_3_CLIENT) != 0) {
    bela::FPrintF(stderr, L"\x1b[33m* SSL connection using TLSv1.3 / %s\x1b[0m\n", si.CipherInfo.szCipherSuite);
  }
}

void response_trace(minimal_response &resp) {
  int color = 31;
  if (resp.status_code < 200) {
    color = 33;
  } else if (resp.status_code < 300) {
    color = 35;
  } else if (resp.status_code < 400) {
    color = 36;
  }
  std::wstring_view version = L"\x1b[33m1.1";
  if (resp.version == protocol_version::HTTP2) {
    version = L"\x1b[33m2.0";
  } else if (resp.version == protocol_version::HTTP3) {
    version = L"\x1b[36m3.0";
  }
  bela::FPrintF(stderr, L"\x1b[33m< \x1b[34mHTTP\x1b[37m/%s \x1b[36m%d \x1b[%dm%s\x1b[0m\n", version, resp.status_code,
                color, resp.status_text);
  for (auto &[k, v] : resp.headers) {
    bela::FPrintF(stderr, L"\x1b[33m< \x1b[36m%s: \x1b[34m%s\x1b[0m\n", k, v);
  }
}

using baulk::net::native::make_net_error_code;
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
  auto flags = u->TlsFlag();
  if (noCache) {
    DbgPrint(L"Indicates that the request should be forwarded to the originating server");
    flags |= WINHTTP_FLAG_REFRESH;
  }
  auto req = conn->open_request(method, u->uri, flags, ec);
  if (!req) {
    return std::nullopt;
  }
  if (insecureMode) {
    req->set_insecure_mode();
  }
  if (!req->write_headers(hkv, cookies, 0, 0, ec)) {
    return std::nullopt;
  }
  if (!req->write_body(body, content_type, ec)) {
    return std::nullopt;
  }
  if (debugMode) {
    if (auto addr = query_remote_address(req->addressof()); addr) {
      bela::FPrintF(stderr, L"\x1b[33mConnecting to %s (%s) %s|:%d connected.\x1b[0m\n", u->host, u->host, *addr,
                    u->nPort);
    }
    connect_trace(req->addressof());
  }
  auto mr = req->recv_minimal_response(ec);
  if (!mr) {
    return std::nullopt;
  }
  if (debugMode) {
    response_trace(*mr);
  }
  std::vector<char> buffer;
  int64_t recv_size = -1;
  auto content_length = native::content_length(mr->headers);
  if (recv_size = req->recv_completely(content_length, buffer, max_body_size, ec); recv_size < 0) {
    return std::nullopt;
  }
  return std::make_optional<Response>(std::move(*mr), std::move(buffer), static_cast<size_t>(recv_size));
}

inline std::filesystem::path make_destination(const download_options &opts, const baulk::net::native::url &u) {
  if (!opts.destination.empty()) {
    return opts.destination;
  }
  return opts.cwd / u.filename;
}

inline bool make_destination_decorous(std::filesystem::path &destination, bool force_overwrite, bela::error_code &ec) {
  if (!std::filesystem::exists(destination)) {
    return true;
  }
  std::error_code e;
  if (force_overwrite) {
    // Solidified delete it
    return true;
  }
  auto filename = destination.filename().replace_extension();
  auto parent = destination.parent_path();
  auto ext = destination.extension();
  for (int i = 1; i < 100; i++) {
    if (auto newPath = parent / bela::StringCat(filename.native(), L"-(", i, L")", ext.native());
        !std::filesystem::exists(newPath, e)) {
      destination = std::move(newPath);
      return true;
    }
  }
  ec = bela::make_error_code(ERROR_FILE_EXISTS, L"'", destination, L"' already exists");
  return false;
}

std::optional<std::filesystem::path> HttpClient::WinGet(std::wstring_view url, const download_options &opts,
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
  auto flags = u->TlsFlag();
  if (noCache) {
    DbgPrint(L"Indicates that the request should be forwarded to the originating server");
    flags |= WINHTTP_FLAG_REFRESH;
  }
  auto req = conn->open_request(L"GET", u->uri, flags, ec);
  if (!req) {
    return std::nullopt;
  }
  if (insecureMode) {
    req->set_insecure_mode();
  }
  auto destination = make_destination(opts, *u);
  auto filePart = net_internal::FilePart::MakeFilePart(destination, opts.hash_value, ec);
  if (!filePart) {
    return std::nullopt;
  }
  // detect part download
  if (!req->write_headers(hkv, cookies, filePart->CurrentBytes(), filePart->FileSize(), ec)) {
    return std::nullopt;
  }
  if (!req->write_body(L"", L"", ec)) {
    return std::nullopt;
  }
  if (debugMode) {
    if (auto addr = query_remote_address(req->addressof()); addr) {
      bela::FPrintF(stderr, L"\x1b[33mConnecting to %s (%s) %s|:%d connected.\x1b[0m\n", u->host, u->host, *addr,
                    u->nPort);
    }
    connect_trace(req->addressof());
  }
  auto mr = req->recv_minimal_response(ec);
  if (!mr) {
    return std::nullopt;
  }
  if (debugMode) {
    response_trace(*mr);
  }
  // destination not set
  if (opts.destination.empty()) {
    if (auto dispositionName = native::extract_filename(mr->headers); dispositionName) {
      DbgPrint(L"filename from 'Content-Disposition': %v", *dispositionName);
      destination = opts.cwd / *dispositionName;
    }
  }
  if (!make_destination_decorous(destination, opts.OverwriteExists(), ec)) {
    return std::nullopt;
  }
  filePart->RenameTo(destination);

  int64_t total_size = native::content_length(mr->headers);
  bool part_support = !opts.hash_value.empty() && native::enable_part_download(mr->headers) && total_size > 0;
  DbgPrint(L"%s support part download: %v", u->filename, part_support);
  if (!mr->IsSuccessStatusCode()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"response: ", mr->status_code, L" status: ", mr->status_text);
    return std::nullopt;
  }
  if (mr->status_code != 206) {
    if (!filePart->Truncated(ec)) {
      return std::nullopt;
    }
    // else:  // part download support
  } else {
    DbgPrint(L"%s download from bytes: %d", u->filename, filePart->CurrentBytes());
  }
  // Pare progress bar
  baulk::ProgressBar bar;
  if (total_size > 0) {
    bar.Maximum(static_cast<uint64_t>(total_size));
  }
  bar.FileName(destination.filename().native());
  bar.Execute();
  auto finish = bela::finally([&] {
    // finish progressbar
    bar.Finish();
  });
  int64_t current_bytes = filePart->CurrentBytes();

  auto save_part_overlay = [&] {
    if (!part_support) {
      return;
    }
    bela::error_code discard_ec;
    filePart->SaveOverlayData(opts.hash_value, total_size, current_bytes, discard_ec);
    DbgPrint(L"%s download broken for bytes: %d-%d", u->filename, current_bytes, total_size);
  };
  // recv data
  std::vector<char> buffer;
  buffer.reserve(64 * 1024);
  DWORD dwSize = 0;
  do {
    DWORD downloaded_size = 0;
    if (WinHttpQueryDataAvailable(req->addressof(), &dwSize) != TRUE) {
      ec = bela::make_system_error_code();
      save_part_overlay();
      bar.MarkFault();
      return std::nullopt;
    }
    if (buffer.size() < dwSize) {
      buffer.resize(static_cast<size_t>(dwSize) * 2);
    }
    if (WinHttpReadData(req->addressof(), (LPVOID)buffer.data(), dwSize, &downloaded_size) != TRUE) {
      ec = make_net_error_code();
      save_part_overlay();
      bar.MarkFault();
      return std::nullopt;
    }
    filePart->WriteFull(buffer.data(), static_cast<size_t>(downloaded_size), ec);
    current_bytes += dwSize;
    bar.Update(current_bytes);
  } while (dwSize > 0);

  if (total_size != 0 && current_bytes < total_size) {
    bar.MarkFault();
    bar.MarkCompleted();
    ec = bela::make_error_code(bela::ErrGeneral, L"connection has been disconnected");
    save_part_overlay();
    return std::nullopt;
  }
  filePart->Solidified(ec);
  bar.MarkCompleted();
  return std::make_optional(std::move(destination));
}
} // namespace baulk::net