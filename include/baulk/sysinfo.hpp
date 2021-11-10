#ifndef BAULK_SYSINFO_HPP
#define BAULK_SYSINFO_HPP
#include <bela/base.hpp>
#include <bela/codecvt.hpp>

#if defined(_M_X64) || defined(_M_X86)
#include <intrin.h>
#elif defined(_M_ARM64)
#include <arm64_neon.h>
#else
#endif

namespace baulk::osinfo {

struct cpu_info {
  std::wstring brand;
  int cores{0};
  int threads{0};
};

#if defined(_M_X64) || defined(_M_X86)
inline std::wstring resolve_cpu_brand() {
  int cpubrand[4 * 3];
  __cpuid(&cpubrand[0], 0x80000002);
  __cpuid(&cpubrand[4], 0x80000003);
  __cpuid(&cpubrand[8], 0x80000004);
  auto cpu_brand = reinterpret_cast<const char *>(cpubrand);
  return bela::encode_into<char, wchar_t>(
      {reinterpret_cast<const char *>(cpubrand), strnlen(cpu_brand, sizeof(cpubrand))});
}

#elif defined(_M_ARM64)
// _ReadStatusReg
inline std::wstring resolve_cpu_brand() { return L"ARM64"; }
#else
inline std::wstring resolve_cpu_brand() { return L"unknown"; }
#endif

struct monitor {
  std::wstring device_name;
  uint32_t width{0};
  uint32_t height{0};
  int frequency{0};   // 60Hz
  int pixels{0};      // dpi
  int orientation{0}; // 0° 90° 180°
  // Resolution:
  std::wstring operator()() {
    if (orientation != 0) {
      return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels:", pixels, L" Orientation: ",
                             orientation, L"°>");
    }
    return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels:", pixels, L">");
  }
};
class SystemViewer {
public:
  SystemViewer() = default;

private:
};
} // namespace baulk::osinfo

#endif