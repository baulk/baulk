///
#include <baulkrev.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <jsonex.hpp>
#include "net.hpp"
#include "baulk.hpp"

namespace baulk {

constexpr const std::wstring_view archfilesuffix() {
#if defined(_M_X64)
  return L"win-x64.zip";
#elif defined(_M_X86)
  return L"win-x86.zip";
#elif defined(_M_ARM64)
  return L"win-arm64.zip";
#elif defined(_M_ARM)
  return L"win-arm.zip";
#else
  return L"unknown cpu architecture";
#endif
}

// https://api.github.com/repos/baulk/baulk/releases/latest todo
bool ReleaseIsUpgradable(std::wstring &url) {
  std::wstring_view releasename(BAULK_RELEASE_NAME);
  constexpr std::wstring_view releaseprefix = L"refs/tags/";
  if (!bela::StartsWith(releasename, releaseprefix)) {
    baulk::DbgPrint(L"ref name not release %s", releasename);
    return false;
  }
  auto release = bela::StripPrefix(releasename, releaseprefix);
  baulk::DbgPrint(L"detect current release %s", release);
  bela::error_code ec;
  auto resp = baulk::net::RestGet(
      L"https://api.github.com/repos/baulk/baulk/releases/latest", ec);
  if (!resp) {
    bela::FPrintF(stderr,
                  L"baulk upgrade self get metadata: \x1b[31m%s\x1b[0m\n",
                  ec.message);
    return false;
  }
  try {
    /* code */
    auto obj = nlohmann::json::parse(resp->body);
    auto tagname = bela::ToWide(obj["tag_name"].get<std::string_view>());
    if (release == tagname) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m", release);
      return false;
    }
    auto it = obj.find("assets");
    if (it == obj.end()) {
      bela::FPrintF(stderr,
                    L"\x1b[33mbaulk/%s build is not yet complete\x1b[0m",
                    tagname);
      return false;
    }
    for (const auto &p : it.value()) {
      auto name = bela::ToWide(p["name"].get<std::string_view>());
      if (!bela::EndsWithIgnoreCase(name, archfilesuffix())) {
        continue;
      }
      url = bela::ToWide(p["browser_download_url"].get<std::string_view>());
      return true;
    }
    //
  } catch (const std::exception &e) {
    bela::FPrintF(stderr,
                  L"baulk upgrade self decode metadata: \x1b[31m%s\x1b[0m\n",
                  e.what());
    return false;
  }
  return false;
}

int BaulkUpgradeSelf() {
  std::wstring url;
  if (!ReleaseIsUpgradable(url)) {
    return 0;
  }
  bela::FPrintF(stderr, L"\x1b[32mNew release url %s\x1b[0m\n", url);
  return 0;
}

} // namespace baulk