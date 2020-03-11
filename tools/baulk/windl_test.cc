#include <bela/base.hpp>
#include <bela/stdwriter.hpp>
#include <bela/comutils.hpp>
#include <xml.hpp>
#include "net.hpp"

namespace baulk {
bool IsDebugMode = false;
constexpr size_t UerAgentMaximumLength = 64;
wchar_t UserAgent[UerAgentMaximumLength] = L"Wget/5.0 (Baulk)";
} // namespace baulk

namespace baulk::net {
std::optional<std::wstring> WinGet(std::wstring_view url,
                                   std::wstring_view workdir,
                                   bool forceoverwrite, bela::error_code ec);
} // namespace baulk::net
int download_atom() {
  bela::error_code ec;
  auto resp = baulk::net::RestGet(
      L"https://github.com/baulk/bucket/commits/master.atom", ec);
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

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    return download_atom();
  }
  bela::error_code ec;
  auto file = baulk::net::WinGet(argv[1], L".", true, ec);
  if (!file) {
    bela::FPrintF(stderr, L"download: %s error: %s\n", argv[1], ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"download: %s => %s\n", argv[1], *file);
  return 0;
}