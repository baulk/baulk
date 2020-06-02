//
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <winhttp.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "baulk.hpp"
#include "indicators.hpp"
#include "net.hpp"

namespace baulk::net {

inline bela::error_code make_net_error_code(std::wstring_view prefix = L"") {
  bela::error_code ec;
  ec.code = GetLastError();
  ec.message = bela::resolve_module_error_message(L"winhttp.dll", ec.code, prefix);
  return ec;
}

inline void Free(HINTERNET &h) {
  if (h != nullptr) {
    WinHttpCloseHandle(h);
  }
}

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
  static std::optional<FilePart> MakeFilePart(std::wstring_view p, bela::error_code &ec) {
    FilePart file;
    file.path = bela::PathAbsolute(p); // Path cleanup
    auto part = bela::StringCat(file.path, L".part");
    file.FileHandle =
        ::CreateFileW(part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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

struct UrlComponets {
  std::wstring host;
  std::wstring filename;
  std::wstring uri;
  int nPort{80};
  int nScheme{INTERNET_SCHEME_HTTPS};
  inline auto TlsFlag() const { return nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0; }
};

inline std::wstring UrlPathName(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  return std::wstring(pv.back());
}

inline bool CrackUrl(std::wstring_view url, UrlComponets &uc) {
  URL_COMPONENTSW urlcomp;
  ZeroMemory(&urlcomp, sizeof(urlcomp));
  urlcomp.dwStructSize = sizeof(urlcomp);
  urlcomp.dwSchemeLength = (DWORD)-1;
  urlcomp.dwHostNameLength = (DWORD)-1;
  urlcomp.dwUrlPathLength = (DWORD)-1;
  urlcomp.dwExtraInfoLength = (DWORD)-1;
  if (WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), 0, &urlcomp) != TRUE) {
    return false;
  }
  uc.host.assign(urlcomp.lpszHostName, urlcomp.dwHostNameLength);
  std::wstring_view urlpath{urlcomp.lpszUrlPath, urlcomp.dwUrlPathLength};
  uc.filename = UrlPathName(urlpath);
  uc.uri =
      bela::StringCat(urlpath, std::wstring_view(urlcomp.lpszExtraInfo, urlcomp.dwExtraInfoLength));
  uc.nPort = urlcomp.nPort;
  uc.nScheme = urlcomp.nScheme;
  return true;
}

inline void EnableTlsProxy(HINTERNET hSession) {
  auto https_proxy_env = bela::GetEnv(L"HTTPS_PROXY");
  if (!https_proxy_env.empty()) {
    WINHTTP_PROXY_INFOW proxy;
    proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy.lpszProxy = https_proxy_env.data();
    proxy.lpszProxyBypass = nullptr;
    WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxy, sizeof(proxy));
  }
  DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
  WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols,
                   sizeof(secure_protocols));
}

inline bool BodyLength(HINTERNET hReq, uint64_t &len) {
  wchar_t conlen[32];
  DWORD dwXsize = sizeof(conlen);
  if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, conlen,
                          &dwXsize, WINHTTP_NO_HEADER_INDEX) == TRUE) {
    return bela::SimpleAtoi({conlen, dwXsize / 2}, &len);
  }
  return false;
}

inline constexpr int dumphex(unsigned char ch) {
  constexpr int hexval_table[] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0~0f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10~1f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 20~2f
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, // 30~3f
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 40~4f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 50~5f
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 60~6f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 70~7f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80~8f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 90~9f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // a0~af
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // b0~bf
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // c0~cf
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // d0~df
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // e0~ef
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // f0~ff
  };
  return hexval_table[ch];
}

inline int decodehlen2(unsigned char ch0, unsigned char ch1) {
  auto c1 = dumphex(ch0);
  auto c2 = dumphex(ch1);
  if (c1 < 0 || c2 < 0) {
    return -1;
  }
  return ((c1 << 4) | c2);
}

std::string UrlDecode(std::wstring_view str) {
  std::string buf;
  buf.reserve(str.size());
  auto p = str.data();
  auto len = str.size();
  for (size_t i = 0; i < len; i++) {
    auto ch = p[i];
    if (ch != '%') {
      buf.push_back(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    auto c1 = static_cast<unsigned char>(p[i + 1]);
    auto c2 = static_cast<unsigned char>(p[i + 2]);
    auto n = decodehlen2(c1, c2);
    if (n > 0) {
      buf.push_back(static_cast<char>(n));
    }
    i += 2;
  }
  return buf;
}

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
// update filename
inline bool Disposition(HINTERNET hReq, std::wstring &fn) {
  wchar_t diposition[MAX_PATH + 4];
  DWORD dwXsize = sizeof(diposition);
  if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CUSTOM, L"Content-Disposition", diposition, &dwXsize,
                          WINHTTP_NO_HEADER_INDEX) != TRUE) {
    return false;
  }
  std::vector<std::wstring_view> pvv =
      bela::StrSplit(diposition, bela::ByChar(';'), bela::SkipEmpty());
  constexpr std::wstring_view fns = L"filename=";
  constexpr std::wstring_view fnsu = L"filename*=";
  for (auto e : pvv) {
    auto s = bela::StripAsciiWhitespace(e);
    if (bela::ConsumePrefix(&s, fns)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      fn = s;
      return true;
    }
    if (bela::ConsumePrefix(&s, fnsu)) {
      bela::ConsumePrefix(&s, L"\"");
      bela::ConsumeSuffix(&s, L"\"");
      fn = bela::ToWide(UrlDecode(s));
      return true;
    }
  }
  return false;
}

std::wstring flatten_http_headers(const headers_t &headers,
                                  const std::vector<std::wstring> &cookies) {
  std::wstring flattened_headers;
  for (const auto &[key, value] : headers) {
    bela::StrAppend(&flattened_headers, key, L": ", value, L"\r\n");
  }
  if (!cookies.empty()) {
    bela::StrAppend(&flattened_headers, L"Cookie: ", bela::StrJoin(cookies, L"; "), L"\r\n");
  }
  return flattened_headers;
}

static inline void query_header_length(HINTERNET request_handle, DWORD header, DWORD &length) {
  WinHttpQueryHeaders(request_handle, header, WINHTTP_HEADER_NAME_BY_INDEX,
                      WINHTTP_NO_OUTPUT_BUFFER, &length, WINHTTP_NO_HEADER_INDEX);
}

void Response::ParseHeadersString(std::wstring_view hdr) {
  constexpr std::wstring_view content_type = L"Content-Type";
  std::vector<std::wstring_view> hlines =
      bela::StrSplit(hdr, bela::ByString(L"\r\n"), bela::SkipEmpty());
  for (const auto ln : hlines) {
    if (auto pos = ln.find(':'); pos != std::wstring_view::npos) {
      auto k = bela::StripTrailingAsciiWhitespace(ln.substr(0, pos));
      auto v = bela::StripTrailingAsciiWhitespace(ln.substr(pos + 1));
      hkv.emplace(k, v);
    }
  }
}

std::optional<Response> HttpClient::WinRest(std::wstring_view method, std::wstring_view url,
                                            std::wstring_view contenttype, std::wstring_view body,
                                            bela::error_code &ec) {
  HINTERNET hSession = nullptr;
  HINTERNET hConnect = nullptr;
  HINTERNET hRequest = nullptr;
  auto closer = bela::final_act([&] {
    Free(hSession);
    Free(hConnect);
    Free(hRequest);
  });
  UrlComponets uc;
  if (!CrackUrl(url, uc)) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  hSession = WinHttpOpen(baulk::UserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (hSession == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  EnableTlsProxy(hSession);
  hConnect = WinHttpConnect(hSession, uc.host.data(), uc.nPort, 0);
  if (hConnect == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  hRequest = WinHttpOpenRequest(hConnect, method.data(), uc.uri.data(), nullptr, WINHTTP_NO_REFERER,
                                WINHTTP_DEFAULT_ACCEPT_TYPES, uc.TlsFlag());
  if (hRequest == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  if (!hkv.empty()) {
    auto flattened_headers = flatten_http_headers(hkv, cookies);
    if (WinHttpAddRequestHeaders(hRequest, flattened_headers.data(),
                                 static_cast<size_t>(flattened_headers.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
  }
  if (!body.empty()) {
    auto addheader =
        bela::StringCat(L"Content-Type: ", contenttype.empty() ? contenttype : L"text/plain",
                        L"\r\nContent-Length: ", body.size(), L"\r\n");
    if (WinHttpAddRequestHeaders(hRequest, addheader.data(), static_cast<size_t>(addheader.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           const_cast<LPVOID>(reinterpret_cast<LPCVOID>(body.data())), body.size(),
                           body.size(), 0) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
  } else {
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
                           0, 0) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
  }

  if (WinHttpReceiveResponse(hRequest, nullptr) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  Response resp;
  DWORD headerBufferLength = 0;
  query_header_length(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, headerBufferLength);
  std::string hdbf;
  hdbf.resize(headerBufferLength);
  if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX,
                          hdbf.data(), &headerBufferLength, WINHTTP_NO_HEADER_INDEX) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  resp.ParseHeadersString(
      std::wstring_view{reinterpret_cast<const wchar_t *>(hdbf.data()), headerBufferLength / 2});
  std::vector<char> readbuf;
  readbuf.reserve(64 * 1024);
  DWORD dwSize = sizeof(resp.statuscode);
  if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr,
                          &resp.statuscode, &dwSize, nullptr) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  do {
    DWORD downloaded_size = 0;
    if (WinHttpQueryDataAvailable(hRequest, &dwSize) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    if (readbuf.size() < dwSize) {
      readbuf.resize(static_cast<size_t>(dwSize) * 2);
    }
    if (WinHttpReadData(hRequest, (LPVOID)readbuf.data(), dwSize, &downloaded_size) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
    resp.body.append(readbuf.data(), dwSize);
  } while (dwSize > 0);
  return std::make_optional(std::move(resp));
}

std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view workdir,
                                   bool forceoverwrite, bela::error_code &ec) {
  HINTERNET hSession = nullptr;
  HINTERNET hConnect = nullptr;
  HINTERNET hRequest = nullptr;
  auto closer = bela::final_act([&] {
    Free(hSession);
    Free(hConnect);
    Free(hRequest);
  });
  UrlComponets uc;
  if (!CrackUrl(url, uc)) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  hSession = WinHttpOpen(baulk::UserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (hSession == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  EnableTlsProxy(hSession);
  hConnect = WinHttpConnect(hSession, uc.host.data(), uc.nPort, 0);
  if (hConnect == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  hRequest = WinHttpOpenRequest(hConnect, L"GET", uc.uri.data(), nullptr, WINHTTP_NO_REFERER,
                                WINHTTP_DEFAULT_ACCEPT_TYPES, uc.TlsFlag());
  if (hRequest == nullptr) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0,
                         0) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  if (WinHttpReceiveResponse(hRequest, nullptr) != TRUE) {
    ec = make_net_error_code();
    return std::nullopt;
  }
  baulk::ProgressBar bar;
  uint64_t blen = 0;
  if (BodyLength(hRequest, blen)) {
    bar.Maximum(blen);
  }
  Disposition(hRequest, uc.filename);

  auto dest = bela::PathCat(workdir, uc.filename);
  if (bela::PathExists(dest)) {
    if (!forceoverwrite) {
      ec = bela::make_error_code(ERROR_FILE_EXISTS, L"'", dest, L"' already exists");
      return std::nullopt;
    }
    if (DeleteFileW(dest.data()) != TRUE) {
      ec = make_net_error_code();
      return std::nullopt;
    }
  }
  size_t total_downloaded_size = 0;
  DWORD dwSize = 0;
  std::vector<char> buf;
  buf.reserve(64 * 1024);
  auto file = FilePart::MakeFilePart(dest, ec);
  bar.FileName(uc.filename);
  bar.Execute();
  auto finish = bela::finally([&] {
    // finish progressbar
    bar.Finish();
  });
  do {
    DWORD downloaded_size = 0;
    if (WinHttpQueryDataAvailable(hRequest, &dwSize) != TRUE) {
      ec = make_net_error_code();
      bar.MarkFault();
      return std::nullopt;
    }
    if (buf.size() < dwSize) {
      buf.resize(static_cast<size_t>(dwSize) * 2);
    }
    if (WinHttpReadData(hRequest, (LPVOID)buf.data(), dwSize, &downloaded_size) != TRUE) {
      ec = make_net_error_code();
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

constexpr auto MaximumTime = (std::numeric_limits<std::uint64_t>::max)();

std::uint64_t UrlResponseTime(std::wstring_view url) {
  UrlComponets uc;
  if (!CrackUrl(url, uc)) {
    return MaximumTime;
  }
  auto begin = std::chrono::steady_clock::now();
  bela::error_code ec;
  if (auto conn = baulk::net::DialTimeout(uc.host, uc.nPort, 10000, ec); !conn) {
    return MaximumTime;
  }
  auto cur = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(cur - begin).count();
}

std::wstring_view BestUrlInternal(const std::vector<std::wstring> &urls) {
  if (urls.empty()) {
    return L"";
  }
  if (urls.size() == 1) {
    return urls[0];
  }
  auto elapsed = MaximumTime;
  size_t pos = 0;
  auto suffix = bela::StringCat(L"#", baulk::BaulkLocale());
  // The first round to determine whether there is a mirror image of the area
  for (const auto &u : urls) {
    std::wstring_view url(u);
    if (bela::EndsWithIgnoreCase(url, suffix)) {
      // eg: https://npm.taobao.org/.../MinGit-2.26.0-busybox-64-bit.zip#zh-CN
      url.remove_suffix(suffix.size());
      return url;
    }
  }
  // Second round of analysis of network connection establishment time
  for (size_t i = 0; i < urls.size(); i++) {
    // connect url to get elapsed timeout
    auto resptime = UrlResponseTime(urls[i]);
    if (resptime < elapsed) {
      pos = i;
    }
  }
  return urls[pos];
}

std::wstring_view BestUrl(const std::vector<std::wstring> &urls) {
  auto url = BestUrlInternal(urls);
  if (auto pos = url.find('#'); pos != std::wstring_view::npos) {
    return url.substr(0, pos);
  }
  return url;
}

std::wstring_view UrlFileName(std::wstring_view url) {
  auto pos = url.find_last_not_of('/');
  if (pos == std::wstring_view::npos) {
    return L"";
  }
  url.remove_suffix(url.size() - pos - 1);
  if (pos = url.rfind('/'); pos != std::wstring_view::npos) {
    url.remove_prefix(pos + 1);
  }
  if (pos = url.find_first_of(L"?#"); pos != std::wstring::npos) {
    return url.substr(0, pos);
  }
  return url;
}

} // namespace baulk::net
