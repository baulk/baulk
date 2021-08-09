//
#include <bela/terminal.hpp>
#include "pkg.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {
int pkg_info(std::wstring_view name) {
  bela::error_code ec, oec;
  auto pkg = baulk::bucket::PackageMetaEx(name, ec);
  if (!pkg) {
    bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
    return 1;
  }
  auto lopkg = baulk::bucket::PackageLocalMeta(name, oec);
  bela::FPrintF(stderr, L"Name:        \x1b[32m%s\x1b[0m\n"
                         "Bucket:      \x1b[34m%s\x1b[0m\n"
                         "Description: %s\n"
    , pkg->name, pkg->bucket, pkg->description);
  if (lopkg) {
    bela::FPrintF(stderr, L"Version:     %s [installed \x1b[33m%s\x1b[0m]\n"
                         "Installed:   Yes\n"
    , pkg->version, lopkg->version);
  } else{
    bela::FPrintF(stderr, L"Version:     %s\n"
                         "Installed:   No\n"
    , pkg->version);
  }
  bela::FPrintF(stderr, L"Urls:\n");
  for (auto it : pkg->urls) {
    bela::FPrintF(stderr, L"  %s\n", it);
  }
  if (!pkg->links.empty()) {
    bela::FPrintF(stderr, L"Links:\n");
    for (auto it : pkg->links) {
      bela::FPrintF(stderr, L"  %s\n", it.path);
    }
  }
  if (!pkg->launchers.empty()) {
    bela::FPrintF(stderr, L"Launchers:\n");
    for (auto it : pkg->launchers) {
      bela::FPrintF(stderr, L"  %s\n", it.path);
    }
  }

  return 0;
}

void usage_info() {
    bela::FPrintF(stderr, LR"(Usage: baulk info [package]...
Show package information

Example:
  baulk info wget

)");
}

int cmd_info(const argv_t &argv) {
  if (argv.empty()) {
    usage_info();
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk info: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  for (auto pkg : argv) {
    pkg_info(pkg);
    bela::FPrintF(stderr, L"\n");
  }
  return 0;
}
} // namespace baulk::commands
