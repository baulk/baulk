//
#include <bela/env.hpp>
#include <baulk/net/client.hpp>
#include <baulk/indicators.hpp>
#include "native.hpp"
#include "file.hpp"

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
  if (!req->write_headers(hkv, cookies, 0, 0, ec)) {
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
  auto content_length = native::content_length(mr->headers);
  if (recv_size = req->recv_completely(content_length, buffer, direct_max_body_size, ec); recv_size < 0) {
    return std::nullopt;
  }
  return std::make_optional<Response>(std::move(*mr), std::move(buffer), static_cast<size_t>(recv_size));
}

// WinGet download file
std::optional<std::wstring> HttpClient::WinGet(std::wstring_view url, std::wstring_view cwd,
                                               std::wstring_view hash_value, bool force_overwrite,
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
  auto req = conn->open_request(L"GET", u->uri, u->TlsFlag(), ec);
  if (!req) {
    return std::nullopt;
  }
  if (insecureMode) {
    req->set_insecure_mode();
  }
  auto destination = bela::PathCat(cwd, u->filename);

  auto filePart = net_internal::FilePart::MakeFilePart(destination, hash_value, ec);
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
  auto mr = req->recv_minimal_response(ec);
  if (!mr) {
    return std::nullopt;
  }
  if (auto newName = native::extract_filename(mr->headers); newName) {
    destination = bela::PathCat(cwd, *newName);
    filePart->RenameTo(destination);
  }
  if (bela::PathExists(destination)) {
    if (!force_overwrite) {
      ec = bela::make_error_code(ERROR_FILE_EXISTS, L"'", destination, L"' already exists");
      return std::nullopt;
    }
    if (DeleteFileW(destination.data()) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
  }

  int64_t total_size = native::content_length(mr->headers);
  bool part_support = !hash_value.empty() && native::enable_part_download(mr->headers) && total_size > 0;
  if (!mr->IsSuccessStatusCode()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"response: ", mr->status_code, L" status: ", mr->status_text);
    return std::nullopt;
  }
  if (mr->status_code == 206) {
    /// DEBUG for
  } else {
    if (!filePart->Truncated(ec)) {
      return std::nullopt;
    }
  }
  baulk::ProgressBar bar;
  if (total_size > 0) {
    bar.Maximum(static_cast<uint64_t>(total_size));
  }
  bar.FileName(bela::BaseName(destination));
  bar.Execute();
  auto finish = bela::finally([&] {
    // finish progressbar
    bar.Finish();
  });
  int64_t current_bytes = filePart->CurrentBytes();
  std::vector<char> buffer;
  buffer.reserve(64 * 1024);
  DWORD dwSize = 0;
  auto save_part_overlay = [&] {
    if (!part_support) {
      return;
    }
    bela::error_code discard_ec;
    filePart->SaveOverlayData(hash_value, total_size, current_bytes, discard_ec);
  };
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
  filePart->UtilizeFile(ec);
  bar.MarkCompleted();
  return std::make_optional(std::move(destination));
}
} // namespace baulk::net