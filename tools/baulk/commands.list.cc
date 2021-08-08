//
#include "baulk.hpp"
#include "fs.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {

inline std::wstring StringCategory(baulk::Package &pkg) {
  if (pkg.venv.category.empty()) {
    return L"";
  }
  return bela::StringCat(L" \x1b[36m[", pkg.venv.category, L"]\x1b[0m");
}

// check upgradable
int cmd_list_all() {
  bela::fs::Finder finder;
  bela::error_code ec;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  size_t upgradable = 0;
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
        upgradable++;
        bela::FPrintF(stderr,
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                      opkg->name, opkg->bucket, opkg->version, pkg.version, pkg.bucket,
                      baulk::BaulkIsFrozenPkg(pkgname) ? L" \x1b[33m(frozen)\x1b[0m" : L"", StringCategory(*opkg));
        continue;
      }
      bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n", opkg->name, opkg->bucket, opkg->version,
                    StringCategory(*opkg));
    } while (finder.Next());
  }
  bela::FPrintF(stderr, L"\x1b[32m%d packages can be updated.\x1b[0m\n", upgradable);
  return 0;
}

void usage_list() {
  bela::FPrintF(stderr, LR"(Usage: baulk list [package]...
List installed packages based on package names.

Example:
  baulk list
  baulk list curl

)");
}

int cmd_list(const argv_t &argv) {
  if (argv.empty()) {
    return cmd_list_all();
  }
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  bela::error_code ec;
  for (const auto a : argv) {
    auto opkg = baulk::bucket::PackageLocalMeta(a, ec);
    if (!opkg) {
      baulk::DbgPrint(L"list package '%s' error: %s", a, ec.message);
      continue;
    }
    baulk::Package pkg;
    if (baulk::bucket::PackageUpdatableMeta(*opkg, pkg)) {
      bela::FPrintF(stderr,
                    L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                    L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                    opkg->name, opkg->bucket, opkg->version, pkg.version, pkg.bucket,
                    baulk::BaulkIsFrozenPkg(a) ? L" \x1b[33m(frozen)\x1b[0m" : L"", StringCategory(*opkg));
      continue;
    }
    bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n", opkg->name, opkg->bucket, opkg->version,
                  StringCategory(*opkg));
  }
  return 0;
}
} // namespace baulk::commands
