//
#include <bela/stdwriter.hpp>
#include <version.hpp>
#include "pkg.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
constexpr const std::wstring_view Architecture() {
#if defined(_M_X64)
  return L"x64";
#elif defined(_M_X86)
  return L"x86";
#elif defined(_M_ARM64)
  return L"ARM64";
#elif defined(_M_ARM)
  return L"ARM";
#else
  return L"UNKNOWN";
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
    bela::FPrintF(stderr, L"baulk: '%s' does not support \x1b[31m%s\x1b[0m\n",
                  name, Architecture());
    return 1;
  }
  return baulk::package::BaulkInstall(*pkg);
}

int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr,
                  L"usage: baulk install package\nInstall specific packages. "
                  L"upgrade or repair installation if already installed\n");
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