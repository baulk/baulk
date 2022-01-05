#include <bela/base.hpp>
#include <appmodel.h>
#include <bela/terminal.hpp>

std::optional<std::wstring> PackageFamilyName(bela::error_code &ec) {
  UINT32 len = 0;
  auto rc = GetCurrentPackageFamilyName(&len, 0);
  if (rc == APPMODEL_ERROR_NO_PACKAGE) {
    ec = bela::make_error_code(bela::ErrGeneral, L"no package");
    return std::nullopt;
  }
  std::wstring packageName;
  packageName.resize(len);
  if (rc = GetCurrentPackageFamilyName(&len, packageName.data()) != 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  packageName.resize(len);
  return std::make_optional(std::move(packageName));
}

// bool IsPackaged() {
//   static const bool isPackaged = []() -> bool {
//     try {
//       const auto package = winrt::Windows::ApplicationModel::Package::Current();
//       return true;
//     } catch (...) {
//       return false;
//     }
//   }();
//   return isPackaged;
// }

int wmain() {
  bela::error_code ec;
  if (auto pkgName = PackageFamilyName(ec); pkgName) {
    bela::FPrintF(stderr, L"TODO: %s\n", *pkgName);
    return 0;
  }
  bela::FPrintF(stderr, L"error: %s\n", ec);
  return 1;
}