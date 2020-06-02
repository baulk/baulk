///
#include <baulkrev.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <jsonex.hpp>
#include "net.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "fs.hpp"
#include "bucket.hpp"
#include "pkg.hpp"

namespace baulk::commands {

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
    baulk::DbgPrint(L"%s not public release", releasename);
  }
  auto release = bela::StripPrefix(releasename, releaseprefix);
  baulk::DbgPrint(L"detect current release %s", release);
  bela::error_code ec;
  auto resp = baulk::net::RestGet(L"https://api.github.com/repos/baulk/baulk/releases/latest", ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade self get metadata: \x1b[31m%s\x1b[0m\n", ec.message);
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
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s build is not yet complete\x1b[0m", tagname);
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
    bela::FPrintF(stderr, L"baulk upgrade self decode metadata: \x1b[31m%s\x1b[0m\n", e.what());
    return false;
  }
  return false;
}

void checkself() {
  auto bun = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\baulk-update-new.exe");
  auto bu = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\baulk-update.exe");
  if (bela::PathExists(bun)) {
    MoveFileEx(bun.data(), bu.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
  }
  std::wstring url;
  if (ReleaseIsUpgradable(url)) {
    bela::FPrintF(stderr,
                  L"\x1b[32mNew release url %s\x1b[0m\nPlease run "
                  L"'baulk-update' update baulk\n",
                  url);
  }
}

int cmd_upgrade(const argv_t &argv) {
  if (argv.size() > 0 && argv[0] == L"--self") {
    checkself();
    return 0;
    // return baulk::BaulkUpgradeSelf();
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }

  baulk::fs::Finder finder;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  if (finder.First(locksdir, L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgname = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgname, L".json")) {
        continue;
      }
      pkgname.remove_suffix(5);
      auto opkg = baulk::bucket::PackageLocalMeta(pkgname, ec);
      if (!opkg) {
        continue;
      }
      baulk::Package pkg;
      if (baulk::bucket::PackageUpdatableMeta(*opkg, pkg)) {
        baulk::package::BaulkInstall(pkg);
        continue;
      }
    } while (finder.Next());
  }
  return 0;
}
} // namespace baulk::commands