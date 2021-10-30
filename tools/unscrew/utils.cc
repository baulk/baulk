#include "window.hpp"

namespace baulk::unscrew {
typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)
typedef NTSTATUS(WINAPI *rtl_get_version_t)(PRTL_OSVERSIONINFOW);

bool IsWindowsVersionOrGreater(int major, int minor, int buildNumber) {
  auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) {
    return false;
  }
  auto invoke_ = reinterpret_cast<rtl_get_version_t>(GetProcAddress(ntdll, "RtlGetVersion"));
  if (invoke_ == nullptr) {
    return false;
  }
  RTL_OSVERSIONINFOW rovi = {0};
  rovi.dwOSVersionInfoSize = sizeof(rovi);
  if (invoke_(&rovi) != STATUS_SUCCESS) {
    return false;
  }
  if (auto dwMajorVersion = static_cast<int>(rovi.dwMajorVersion); dwMajorVersion != major) {
    return dwMajorVersion - major > 0;
  }
  if (auto dwMinorVersion = static_cast<int>(rovi.dwMinorVersion); dwMinorVersion != minor) {
    return rovi.dwMinorVersion - minor > 0;
  }
  if (auto dwBuildNumber = static_cast<int>(rovi.dwBuildNumber); dwBuildNumber != buildNumber) {
    return dwBuildNumber - buildNumber > 0;
  }
  return false;
}

} // namespace baulk::unscrew