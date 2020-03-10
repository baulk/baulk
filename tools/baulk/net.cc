//
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <bela/finaly.hpp>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <winhttp.h>
#include <cstdio>
#include <cstdlib>
#include "baulk.hpp"
#include "indicators.hpp"
#include "net.hpp"

namespace baulk::net {
class FilePart {
public:
  FilePart() noexcept = default;
  FilePart(const FilePart &) = delete;
  FilePart &operator=(const FilePart &) = delete;
  FilePart(FilePart &&o) noexcept { transfer_ownership(std::move(o)); }
  FilePart &operator=(FilePart &&o) noexcept {
    transfer_ownership(std::move(o));
    return *this;
  }
  ~FilePart() noexcept { rollback(); }

  bool Finish() {
    if (FileHandle == INVALID_HANDLE_VALUE) {
      SetLastError(ERROR_INVALID_HANDLE);
      return false;
    }
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
    auto part = bela::StringCat(path, L".part");
    return (MoveFileW(part.data(), path.data()) == TRUE);
  }
  bool Write(const char *data, DWORD len) {
    DWORD dwlen = 0;
    if (WriteFile(FileHandle, data, len, &dwlen, nullptr) != TRUE) {
      return false;
    }
    return len == dwlen;
  }
  static std::optional<FilePart> MakeFilePart(std::wstring_view p,
                                              bela::error_code &ec) {
    FilePart file;
    file.path = bela::PathCat(p); // Path cleanup
    auto part = bela::StringCat(file.path, L".part");
    file.FileHandle = ::CreateFileW(
        part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.FileHandle == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    return std::make_optional(std::move(file));
  }

private:
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  std::wstring path;
  void transfer_ownership(FilePart &&other) {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
    }
    FileHandle = other.FileHandle;
    other.FileHandle = INVALID_HANDLE_VALUE;
    path = other.path;
    other.path.clear();
  }
  void rollback() noexcept {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
      FileHandle = INVALID_HANDLE_VALUE;
      auto part = bela::StringCat(path, L".part");
      DeleteFileW(part.data());
    }
  }
};

inline void Free(HINTERNET &h) {
  if (h != nullptr) {
    WinHttpCloseHandle(h);
  }
}

class HttpExecutor {
public:
  HttpExecutor() = default;
  HttpExecutor(const HttpExecutor &) = delete;
  HttpExecutor &operator=(const HttpExecutor &) = delete;
  ~HttpExecutor() {
    Free(hSession);
    Free(hConnect);
    Free(hRequest);
  }
  bool Initialize(bela::error_code &ec);
  bool WriteBody(std::string_view body, bela::error_code &ec) {
    const auto p = body.data();
    size_t total = 0;
    while (total < body.size()) {
      DWORD written = 0;
      if (WinHttpWriteData(hRequest, p + total, body.size() - total,
                           &written) != TRUE) {
        ec = bela::make_system_error_code();
        return false;
      }
      total += written;
    }
    return true;
  }
  bool WriteBody(std::wstring_view body, bela::error_code &ec) {
    return WriteBody(bela::ToNarrow(body), ec);
  }
  bool WriteHeader(std::wstring_view headers, bela::error_code &ec) {
    if (WinHttpAddRequestHeaders(hRequest, headers.data(), headers.size(),
                                 WINHTTP_ADDREQ_FLAG_ADD) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }
  bool BodyLength(uint64_t &len);
  bool Disposition(std::wstring &fn);
  bool Send(std::wstring_view url, bela::error_code &ec);

private:
  HINTERNET hSession{nullptr};
  HINTERNET hConnect{nullptr};
  HINTERNET hRequest{nullptr};
};

bool HttpExecutor::Initialize(bela::error_code &ec) {
  hSession = WinHttpOpen(baulk::UserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (hSession == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  // Enable Proxy
  auto https_proxy_env = bela::GetEnv(L"HTTPS_PROXY");
  if (!https_proxy_env.empty()) {
    WINHTTP_PROXY_INFOW proxy;
    proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy.lpszProxy = https_proxy_env.data();
    proxy.lpszProxyBypass = nullptr;
    WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxy, sizeof(proxy));
  }
  DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
                         WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
  WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols,
                   sizeof(secure_protocols));
  return true;
}

bool HttpExecutor::BodyLength(uint64_t &len) {
  wchar_t conlen[32];
  DWORD dwXsize = sizeof(conlen);
  if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
                          WINHTTP_HEADER_NAME_BY_INDEX, conlen, &dwXsize,
                          WINHTTP_NO_HEADER_INDEX) == TRUE) {
    return bela::SimpleAtoi({conlen, dwXsize / 2}, &len);
  }
  return false;
}

bool HttpExecutor::Disposition(std::wstring &fn) {
  wchar_t diposition[MAX_PATH + 4];
  DWORD dwXsize = sizeof(diposition);
  if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM,
                          L"Content-Disposition", diposition, &dwXsize,
                          WINHTTP_NO_HEADER_INDEX) != TRUE) {
    return false;
  }
  std::vector<std::wstring_view> pvv =
      bela::StrSplit(diposition, bela::ByChar(';'), bela::SkipEmpty());
  constexpr std::wstring_view fns = L"filename=";
  for (auto e : pvv) {
    auto s = bela::StripAsciiWhitespace(e);
    if (bela::ConsumePrefix(&s, fns)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      fn = s;
      return true;
    }
  }
  return false;
}

std::wstring flatten_http_headers(const headers_t &headers,
                                  const std::vector<std::wstring> &cookies) {
  std::wstring flattened_headers;
  for (const auto &[key, value] : headers) {
    flattened_headers.append(key);
    flattened_headers.push_back(':');
    flattened_headers.append(value);
    flattened_headers.append(L"\r\n");
  }
  if (!cookies.empty()) {
    bela::StrAppend(&flattened_headers, L"Cookie: ",
                    bela::StrJoin(cookies, L"; "), L"\r\n");
  }
  return flattened_headers;
}

std::optional<Response> HttpClient::WinRest(std::wstring_view method,
                                            std::wstring_view url,
                                            std::wstring_view contenttype,
                                            std::wstring_view body,
                                            bela::error_code &ec) {
  //
  return std::nullopt;
}

std::optional<std::wstring> WinGet(std::wstring_view url,
                                   std::wstring_view workdir,
                                   bool forceoverwrite, bela::error_code ec) {
  //
  return std::nullopt;
}

inline std::wstring UrlPathName(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  return std::wstring(pv.back());
}

inline bool BodyLength(HINTERNET hReq, uint64_t &len) {
  wchar_t conlen[32];
  DWORD dwXsize = sizeof(conlen);
  if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH,
                          WINHTTP_HEADER_NAME_BY_INDEX, conlen, &dwXsize,
                          WINHTTP_NO_HEADER_INDEX) == TRUE) {
    return bela::SimpleAtoi({conlen, dwXsize / 2}, &len);
  }
  return false;
}

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
// update filename
inline bool Disposition(HINTERNET hReq, std::wstring &fn) {
  wchar_t diposition[MAX_PATH + 4];
  DWORD dwXsize = sizeof(diposition);
  if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CUSTOM, L"Content-Disposition",
                          diposition, &dwXsize,
                          WINHTTP_NO_HEADER_INDEX) != TRUE) {
    return false;
  }
  std::vector<std::wstring_view> pvv =
      bela::StrSplit(diposition, bela::ByChar(';'), bela::SkipEmpty());
  constexpr std::wstring_view fns = L"filename=";
  for (auto e : pvv) {
    auto s = bela::StripAsciiWhitespace(e);
    if (bela::ConsumePrefix(&s, fns)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      fn = s;
      return true;
    }
  }
  return false;
}

std::optional<std::wstring> WinGetInternal(std::wstring_view url,
                                           std::wstring_view workdir,
                                           bool forceoverwrite,
                                           bela::error_code ec) {
  URL_COMPONENTSW urlcomp;
  ZeroMemory(&urlcomp, sizeof(urlcomp));
  urlcomp.dwStructSize = sizeof(urlcomp);
  urlcomp.dwSchemeLength = (DWORD)-1;
  urlcomp.dwHostNameLength = (DWORD)-1;
  urlcomp.dwUrlPathLength = (DWORD)-1;
  urlcomp.dwExtraInfoLength = (DWORD)-1;
  if (WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), 0,
                      &urlcomp) != TRUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  std::wstring_view urlpath{urlcomp.lpszUrlPath, urlcomp.dwUrlPathLength};
  auto filename = UrlPathName(urlpath);
  HINTERNET hSession = nullptr;
  HINTERNET hConnect = nullptr;
  HINTERNET hRequest = nullptr;
  auto deleter = bela::final_act([&] {
    Free(hSession);
    Free(hConnect);
    Free(hRequest);
  });
  hSession =
      WinHttpOpen(L"Wget/5.0 (Baulk)", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (hSession == nullptr) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  auto https_proxy_env = bela::GetEnv(L"HTTPS_PROXY");
  if (!https_proxy_env.empty()) {
    WINHTTP_PROXY_INFOW proxy;
    proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy.lpszProxy = https_proxy_env.data();
    proxy.lpszProxyBypass = nullptr;
    WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxy, sizeof(proxy));
  }
  DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
                         WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
  WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols,
                   sizeof(secure_protocols));
  std::wstring hn(urlcomp.lpszHostName, urlcomp.dwHostNameLength);
  hConnect =
      WinHttpConnect(hSession, hn.data(), INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (hConnect == nullptr) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  auto qu =
      bela::StringCat(urlpath, std::wstring_view{urlcomp.lpszExtraInfo,
                                                 urlcomp.dwExtraInfoLength});
  hRequest = WinHttpOpenRequest(
      hConnect, L"GET", qu.data(), nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (hRequest == nullptr) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != TRUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (WinHttpReceiveResponse(hRequest, nullptr) != TRUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  baulk::ProgressBar bar;
  uint64_t blen = 0;
  if (BodyLength(hRequest, blen)) {
    bar.Maximum(blen);
  }
  Disposition(hRequest, filename);

  auto dest = bela::PathCat(workdir, filename);
  if (bela::PathExists(dest)) {
    if (!forceoverwrite) {
      ec = bela::make_error_code(ERROR_FILE_EXISTS, L"'", dest,
                                 L"' already exists");
      return std::nullopt;
    }
    if (DeleteFileW(dest.data()) != TRUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
  }
  size_t total_downloaded_size = 0;
  DWORD dwSize = 0;
  std::vector<char> buf;
  buf.reserve(64 * 1024);
  auto file = FilePart::MakeFilePart(dest, ec);
  bar.FileName(filename);
  bar.Execute();
  auto finish = bela::finally([&] {
    // finish progressbar
    bar.Finish();
  });
  do {
    DWORD downloaded_size = 0;
    if (WinHttpQueryDataAvailable(hRequest, &dwSize) != TRUE) {
      ec = bela::make_system_error_code();
      bar.MarkFault();
      return std::nullopt;
    }
    if (buf.size() < dwSize) {
      buf.resize(static_cast<size_t>(dwSize) * 2);
    }
    if (WinHttpReadData(hRequest, (LPVOID)buf.data(), dwSize,
                        &downloaded_size) != TRUE) {
      ec = bela::make_system_error_code();
      bar.MarkFault();
      return std::nullopt;
    }
    file->Write(buf.data(), downloaded_size);
    total_downloaded_size += downloaded_size;
    bar.Update(total_downloaded_size);
  } while (dwSize > 0);
  file->Finish();
  bar.MarkCompleted();
  return std::make_optional(std::move(dest));
}
} // namespace baulk::net