#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/comutils.hpp>
#include <bela/path.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <xml.hpp>
#include "net.hpp"

namespace baulk {
bool IsDebugMode = false;
bool IsInsecureMode = false;
constexpr size_t UerAgentMaximumLength = 64;
wchar_t UserAgent[UerAgentMaximumLength] = L"Wget/5.0 (Baulk)";
std::wstring locale;
std::wstring_view BaulkLocale() { return locale; }
} // namespace baulk

namespace baulk::net {
std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                   bela::error_code &ec);
bool ResolveName(std::wstring_view host, int port, PADDRINFOEX4 *rhints, bela::error_code &ec);
} // namespace baulk::net
int download_atom() {
  bela::error_code ec;
  auto resp = baulk::net::RestGet(L"https://github.com/baulk/bucket/commits/master.atom", ec);
  if (!resp) {
    bela::FPrintF(stderr, L"restget error: %s\n", ec.message);
    return 1;
  }
  for (const auto &[k, v] : resp->hkv) {
    bela::FPrintF(stderr, L"\x1b[33m%s\x1b[0m: %s\n", k, v);
  }
  bela::FPrintF(stderr, L"body:\n %s\n", resp->body);
  auto doc = baulk::xml::parse_string(resp->body, ec);
  if (!doc) {
    bela::FPrintF(stderr, L"parse xml error: %s\n", ec.message);
    return false;
  }
  //
  auto entry = doc->child("feed").child("entry");
  std::string_view id{entry.child("id").text().as_string()};
  if (auto pos = id.find('/'); pos != std::string_view::npos) {
    bela::FPrintF(stderr, L"Commit: %s\n", id.substr(pos + 1));
  } else {
    bela::FPrintF(stderr, L"bucket invaild id: %s\n", id);
  }
  auto updated = entry.child("updated").text().as_string();
  bela::FPrintF(stderr, L"Update: %s\n", updated);
  return true;
}
#define MAX_ADDRESS_STRING_LENGTH 64
bool resolve_dns() {
  PADDRINFOEX4 rhints = nullptr;
  bela::error_code ec;
  WCHAR AddrString[MAX_ADDRESS_STRING_LENGTH];
  DWORD AddressStringLength;
  if (!baulk::net::ResolveName(L"github.com", 443, &rhints, ec)) {
    bela::FPrintF(stderr, L"GetAddrInfoExW %s\n", ec.message);
    return false;
  }
  auto p = rhints;
  while (p) {
    AddressStringLength = MAX_ADDRESS_STRING_LENGTH;
    WSAAddressToString(p->ai_addr, (DWORD)p->ai_addrlen, nullptr, AddrString, &AddressStringLength);
    bela::FPrintF(stderr, L"IP Address: %s\n", AddrString);
    p = p->ai_next;
  }
  FreeAddrInfoExW(reinterpret_cast<ADDRINFOEXW *>(rhints)); /// Release
  return true;
}

inline std::wstring UrlPathName(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  return std::wstring(pv.back());
}

int wmain(int argc, wchar_t **argv) {
  bela::FPrintF(stderr, L"filename [%s] [%s]\n", UrlPathName(L"jackson/dl.zip"), UrlPathName(L"../../jack"));
  auto s = bela::resolve_module_error_message(L"winhttp.dll", ERROR_FILE_NOT_FOUND, L"dump ");
  bela::FPrintF(stderr, L"%s\n", s);
  s = bela::resolve_system_error_message(ERROR_FILE_NOT_FOUND, L"dump ");
  bela::FPrintF(stderr, L"%s\n", s); // WSAEINPROGRESS
  s = bela::resolve_system_error_message(WSAEINPROGRESS, L"dump ");
  bela::FPrintF(stderr, L"%s\n", s); // WSAEINPROGRESS
  baulk::locale.resize(64);
  if (auto n = GetUserDefaultLocaleName(baulk::locale.data(), 64); n != 0 && n < 64) {
    baulk::locale.resize(n);
  } else {
    baulk::locale.clear();
  }
  if (argc < 2) {
    return download_atom();
  }
  auto resptime = baulk::net::UrlResponseTime(argv[1]);
  bela::FPrintF(stderr, L"RespTimeout %d ms\n", resptime / 1000'000);
  bela::error_code ec;
  auto file = baulk::net::WinGet(argv[1], L".", true, ec);
  if (!file) {
    bela::FPrintF(stderr, L"download: %s error: %s\n", argv[1], ec.message);
    return 1;
  }
  resolve_dns();
  bela::FPrintF(stderr, L"download: %s => %s\n", argv[1], *file);
  return 0;
}