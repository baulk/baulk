#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/datetime.hpp>
#include <baulk/fs.hpp>
#include <baulk/fsmutex.hpp>
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include "commands.hpp"
#include "baulk.hpp"

namespace baulk::commands {

void usage_freeze() {
  bela::FPrintF(stderr, LR"(Usage: baulk freeze [package]...
Freeze specific package

Example:
  baulk freeze wget

)");
}

void usage_unfreeze() {
  bela::FPrintF(stderr, LR"(Usage: baulk unfreeze [package]...
Undo freeze specific package

Example:
  baulk unfreeze wget

)");
}

int cmd_freeze(const argv_t &argv) {
  if (argv.empty()) {
    usage_freeze();
    return 1;
  }
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk freeze: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  auto jo = json::parse_file(Profile(), ec);
  if (!jo) {
    bela::FPrintF(stderr, L"baulk: unable load profile %s\n", ec);
    return 1;
  }
  auto jv = jo->view();
  std::vector<std::string> pkgs;
  jv.fetch_strings_checked("freeze", pkgs);
  auto freeze = [&](std::string_view pkgName) {
    for (const auto &p : pkgs) {
      if (bela::EqualsIgnoreCase(p, pkgName)) {
        return;
      }
    }
    pkgs.emplace_back(pkgName);
  };

  for (auto pkgName : argv) {
    freeze(bela::encode_into<wchar_t, char>(pkgName));
  }
  jo->obj["freeze"] = pkgs;
  jo->obj["updated"] = bela::FormatTime<char>(bela::Now());
  auto text = jo->obj.dump(4);
  if (!bela::io::WriteTextAtomic(text, Profile(), ec)) {
    bela::FPrintF(stderr, L"baulk: unable update profile: %s\n", ec);
    return false;
  }
  bela::FPrintF(stderr, L"baulk freeze package success, freezed: %d\n", pkgs.size());
  return 0;
}

int cmd_unfreeze(const argv_t &argv) {
  if (argv.empty()) {
    usage_unfreeze();
    return 1;
  }
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk freeze: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  auto jo = json::parse_file(Profile(), ec);
  if (!jo) {
    bela::FPrintF(stderr, L"baulk: unable load profile %s\n", ec);
    return 1;
  }
  auto jv = jo->view();
  std::vector<std::string> pkgs;
  jv.fetch_strings_checked("freeze", pkgs);
  std::vector<std::string> newFrozened;
  // checkout
  auto contains = [&](std::wstring_view pkgName) -> bool {
    for (const auto &p : argv) {
      if (bela::EqualsIgnoreCase(p, pkgName)) {
        return true;
      }
    }
    return false;
  };

  for (auto pkgName : pkgs) {
    if (!contains(bela::encode_into<char, wchar_t>(pkgName))) {
      newFrozened.emplace_back(pkgName);
    }
  }
  jo->obj["freeze"] = newFrozened;
  jo->obj["updated"] = bela::FormatTime<char>(bela::Now());
  auto text = jo->obj.dump(4);
  if (!bela::io::WriteTextAtomic(text, Profile(), ec)) {
    bela::FPrintF(stderr, L"baulk: unable update profile: %s\n", ec);
    return false;
  }
  bela::FPrintF(stderr, L"baulk unfreeze package success, freezed: %d\n", pkgs.size());
  return 0;
}

} // namespace baulk::commands
