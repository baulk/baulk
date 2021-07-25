//
#include <bela/terminal.hpp>
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

int install_pkg(std::wstring_view name) {
  bela::error_code ec;
  auto pkg = baulk::bucket::PackageMetaEx(name, ec);
  if (!pkg) {
    bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
    return 1;
  }
  if (pkg->urls.empty()) {
    bela::FPrintF(stderr, L"baulk: '%s' not support \x1b[31m%s\x1b[0m\n", name, architecture());
    return 1;
  }
  return baulk::package::BaulkInstall(*pkg);
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
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk install: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }
  for (auto pkg : argv) {
    install_pkg(pkg);
  }
  return 0;
}
} // namespace baulk::commands