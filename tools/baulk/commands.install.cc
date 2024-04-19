//
#include <bela/terminal.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include <mutex>
#include "pkg.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
constexpr std::wstring_view architecture() {
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
    bela::FPrintF(stderr, L"baulk install: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  if (!InitializeExecutor(ec)) {
    DbgPrint(L"baulk install: unable initialize compiler executor: %s", ec);
  }
  
  std::once_flag once;
  auto oneInst = [&](std::wstring_view name) -> bool {
    bela::error_code ec;
    auto pkg = baulk::PackageMetaEx(name, ec);
    if (!pkg) {
      if (ec.code != baulk::ErrPackageNotYetPorted) {
        bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec);
        return false;
      }
      std::call_once(once, [&]() {
        bela::FPrintF(stderr, L"\x1b[33mbaulk: '%s' not yet ported, now update buckets and retry it.\x1b[0m\n", name);
        if (UpdateBucket(false) != 0) {
          return;
        }
      });
      if (pkg = baulk::PackageMetaEx(name, ec); !pkg) {
        bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec);
        return false;
      }
    }
    if (pkg->urls.empty()) {
      bela::FPrintF(stderr, L"baulk: '%s' not support \x1b[31m%s\x1b[0m\n", name, architecture());
      return false;
    }
    return baulk::package::Install(*pkg);
  };
  for (auto p : argv) {
    oneInst(p);
  }
  return 0;
}
} // namespace baulk::commands