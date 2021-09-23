//
#include <baulkenv.hpp>
#include <appmodel.h>

namespace baulk::env {
std::optional<std::wstring> PackageFamilyName() {
  auto hProcess = GetCurrentProcess();
  UINT32 len = 0;
  auto rc = GetPackageFamilyName(hProcess, &len, 0);
  if (rc == APPMODEL_ERROR_NO_PACKAGE) {
    return std::nullopt;
  }
  std::wstring packageName;
  packageName.resize(len);
  if (rc = GetPackageFamilyName(hProcess, &len, packageName.data()) != 0) {
    return std::nullopt;
  }
  packageName.resize(len);
  return std::make_optional(std::move(packageName));
}
} // namespace baulk::env