//
#include <bela/terminal.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include "pkg.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
constexpr const std::wstring_view architecture() {
#if defined(_M_X64)
  return L"x64";
#elif defined(_M_X86)
  return L"x86";
#elif defined(_M_ARM64)
  return L"arm64";
#elif defined(_M_ARM)
  return L"arm";
#else
  return L"unknown cpu architecture";
#endif
}

class PackageInstaller {
public:
  PackageInstaller() = default;
  PackageInstaller(const PackageInstaller &) = delete;
  PackageInstaller &operator=(const PackageInstaller &) = delete;
  bool Install(std::wstring_view pkgname);

private:
  void Update(std::wstring_view name);
  bool updated{false};
};

void PackageInstaller::Update(std::wstring_view name) {
  if (updated) {
    return;
  }
  bela::FPrintF(stderr, L"\x1b[33mbaulk: '%s' not yet ported, now update buckets and retry it.\x1b[0m\n", name);
  if (UpdateBucket(false) != 0) {
    return;
  }
  updated = true;
}

bool PackageInstaller::Install(std::wstring_view name) {
  bela::error_code ec;
  auto pkg = baulk::bucket::PackageMetaEx(name, ec);
  if (!pkg) {
    if (ec.code != baulk::bucket::ErrPackageNotYetPorted) {
      bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
      return false;
    }
    Update(name);
    if (pkg = baulk::bucket::PackageMetaEx(name, ec); !pkg) {
      bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
      return 1;
    }
  }
  if (pkg->urls.empty()) {
    bela::FPrintF(stderr, L"baulk: '%s' not support \x1b[31m%s\x1b[0m\n", name, architecture());
    return false;
  }
  return baulk::package::PackageInstall(*pkg);
}

void usage_install() {
  bela::FPrintF(stderr, LR"(Usage: baulk install [package]...
Install specific packages. upgrade if already installed. (alias: i)

Example:
  baulk install wget
  baulk i wget

)");
}

int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    usage_install();
    return 1;
  }
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk install: \x1b[31mbaulk %s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!InitializeExecutor(ec)) {
    DbgPrint(L"baulk install: unable initialize compiler executor: %s", ec.message);
  }
  PackageInstaller installer;
  for (auto pkg : argv) {
    installer.Install(pkg);
  }
  return 0;
}
} // namespace baulk::commands