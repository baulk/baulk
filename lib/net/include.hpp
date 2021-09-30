#ifndef BAULK_NET_NETINTERNAL_HPP
#define BAULK_NET_NETINTERNAL_HPP
#include <bela/path.hpp>
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <bela/phmap.hpp>
#include <winhttp.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <baulk/indicators.hpp>
#include <baulk/tcp.hpp>
#include <baulk/net.hpp>

namespace baulk::net {
// class Executor {
// public:
//   Executor(const Executor &) = delete;
//   Executor &operator=(const Executor &) = delete;

// private:
//   Executor() = default;
//   bela::flat_hash_map<std::wstring, std::wstring> parts;
// };

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
  uc.uri = bela::StringCat(urlpath, std::wstring_view(urlcomp.lpszExtraInfo, urlcomp.dwExtraInfoLength));
  uc.nPort = urlcomp.nPort;
  uc.nScheme = urlcomp.nScheme;
  return true;
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
    file.FileHandle = ::CreateFileW(part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr,
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

} // namespace baulk::net

#endif