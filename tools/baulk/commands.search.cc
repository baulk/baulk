#include <bela/fnmatch.hpp>
#include <bela/ascii.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {

void usage_search() {
  bela::FPrintF(stderr, LR"(Usage: baulk search [package]...
Search in package descriptions.

Example:
  baulk search wget
  baulk search win*
  baulk search *

)");
}

int cmd_search(const argv_t &argv) {
  if (argv.empty()) {
    usage_search();
    return 1;
  }

  std::vector<std::wstring> pattern;
  for (const auto a : argv) {
    pattern.emplace_back(bela::AsciiStrToLower(a));
  }
  auto isMatched = [&](std::wstring_view pkgName) -> bool {
    for (const auto &a : pattern) {
      if (bela::FnMatch(a, pkgName)) {
        return true;
      }
    }
    return false;
  };
  // lldb/kali-rolling 1:9.0-49.1 amd64
  //   Next generation, high-performance debugger
  auto onMatched = [](const Bucket &bucket, std::wstring_view pkgName) -> bool {
    bela::error_code ec;
    auto pkg = baulk::PackageMeta(bucket, pkgName, ec);
    if (!pkg) {
      bela::FPrintF(stderr, L"baulk search: parse package meta error: \x1b[31m%s\x1b[0m\n", ec);
      return false;
    }
    if (baulk::IsDebugMode) {
      bela::FPrintF(stderr, L"\x1b[33m* %v urls:\x1b[0m\n  \x1b[33m%v\x1b[0m\n", pkg->name,
                    bela::StrJoin(pkg->urls, L"\x1b[0m\n  \x1b[33m"));
    }
    auto pkgLocal = baulk::PackageLocalMeta(pkgName, ec);
    if (pkgLocal && bela::EndsWithIgnoreCase(pkgLocal->bucket, pkg->bucket)) {
      bela::FPrintF(stderr,
                    L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s [installed "
                    L"\x1b[33m%s\x1b[0m]%s\n  %s\n",
                    pkg->name, pkg->bucket, pkg->version, pkgLocal->version, StringCategory(*pkg), pkg->description);
      return true;
    }
    bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n  %s\n", pkg->name, pkg->bucket, pkg->version,
                  StringCategory(*pkg), pkg->description);
    return true;
  };
  if (!PackageMatched(isMatched, onMatched)) {
    return 1;
  }
  return 0;
}

} // namespace baulk::commands