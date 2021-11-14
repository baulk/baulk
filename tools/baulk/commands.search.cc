#include <bela/fnmatch.hpp>
#include <bela/ascii.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {
// lldb/kali-rolling 1:9.0-49.1 amd64
//   Next generation, high-performance debugger

class Searcher {
public:
  Searcher(const argv_t &argv_) {
    for (const auto a : argv_) {
      argv.emplace_back(bela::AsciiStrToLower(a));
    }
  }
  bool PackageMatched(std::wstring_view pkgName) {
    for (const auto &a : argv) {
      if (bela::FnMatch(a, pkgName)) {
        return true;
      }
    }
    return false;
  };
  bool SearchMatched(std::wstring_view pkgsUnderBucket, std::wstring_view bucket) {
    DbgPrint(L"search bucket: %s", pkgsUnderBucket);
    bela::fs::Finder finder;
    bela::error_code ec;
    if (!finder.First(pkgsUnderBucket, L"*.json", ec)) {
      return false;
    }
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgName = finder.Name();
      auto pkgMetaPath = bela::StringCat(pkgsUnderBucket, L"\\", pkgName);
      pkgName.remove_suffix(5);
      if (!PackageMatched(bela::AsciiStrToLower(pkgName))) {
        continue;
      }
      auto pkg = baulk::bucket::PackageMeta(pkgMetaPath, pkgName, bucket, ec);
      if (!pkg) {
        bela::FPrintF(stderr, L"baulk search: parse package meta error: \x1b[31m%s\x1b[0m\n", ec);
        continue;
      }
      auto lopkg = baulk::bucket::PackageLocalMeta(pkgName, ec);
      if (lopkg && bela::EndsWithIgnoreCase(lopkg->bucket, pkg->bucket)) {
        bela::FPrintF(stderr,
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s [installed "
                      L"\x1b[33m%s\x1b[0m]%s\n  %s\n",
                      pkg->name, pkg->bucket, pkg->version, lopkg->version, StringCategory(*pkg), pkg->description);
        continue;
      }
      bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n  %s\n", pkg->name, pkg->bucket, pkg->version,
                    StringCategory(*pkg), pkg->description);
    } while (finder.Next());

    return true;
  }

private:
  std::vector<std::wstring> argv;
};

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
  Searcher searcher(argv);
  auto buckets = vfs::AppBuckets();
  DbgPrint(L"buckets: %s", buckets);
  for (const auto &bk : LoadedBuckets()) {
    auto pkgsUnderBucket = bela::StringCat(buckets, L"\\", bk.name, L"\\bucket");
    searcher.SearchMatched(pkgsUnderBucket, bk.name);
  }
  return 0;
}

} // namespace baulk::commands