///
#ifndef BELA_PE_HPP
#define BELA_PE_HPP
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include "base.hpp"
#include "endian.hpp"

namespace bela::pe {
enum class Machine : uint16_t {
  UNKNOWN = 0,
  TARGET_HOST = 0x0001, // Useful for indicating we want to interact with the
                        // host and not a WoW guest.
  I386 = 0x014c,        // Intel 386.
  R3000 = 0x0162,       // MIPS little-endian, 0x160 big-endian
  R4000 = 0x0166,       // MIPS little-endian
  R10000 = 0x0168,      // MIPS little-endian
  WCEMIPSV2 = 0x0169,   // MIPS little-endian WCE v2
  ALPHA = 0x0184,       // Alpha_AXP
  SH3 = 0x01a2,         // SH3 little-endian
  SH3DSP = 0x01a3,
  SH3E = 0x01a4,  // SH3E little-endian
  SH4 = 0x01a6,   // SH4 little-endian
  SH5 = 0x01a8,   // SH5
  ARM = 0x01c0,   // ARM Little-Endian
  THUMB = 0x01c2, // ARM Thumb/Thumb-2 Little-Endian
  ARMNT = 0x01c4, // ARM Thumb-2 Little-Endian
  AM33 = 0x01d3,
  POWERPC = 0x01F0, // IBM PowerPC Little-Endian
  POWERPCFP = 0x01f1,
  IA64 = 0x0200,      // Intel 64
  MIPS16 = 0x0266,    // MIPS
  ALPHA64 = 0x0284,   // ALPHA64
  MIPSFPU = 0x0366,   // MIPS
  MIPSFPU16 = 0x0466, // MIPS
  TRICORE = 0x0520,   // Infineon
  CEF = 0x0CEF,
  EBC = 0x0EBC,   // EFI Byte Code
  AMD64 = 0x8664, // AMD64 (K8)
  M32R = 0x9041,  // M32R little-endian
  ARM64 = 0xAA64, // ARM64 Little-Endian
  CEE = 0xC0EE
};
enum class Subsystem : uint16_t {
  UNKNOWN = 0,
  NATIVE = 1,
  GUI = 2,
  CUI = 3, //
  OS2_CUI = 5,
  POSIX_CUI = 7,
  NATIVE_WINDOWS = 8,
  WINDOWS_CE_GUI = 9,
  EFI_APPLICATION = 10,
  EFI_BOOT_SERVICE_DRIVER = 11,
  EFI_RUNTIME_DRIVER = 12,
  EFI_ROM = 13,
  XBOX = 14,
  WINDOWS_BOOT_APPLICATION = 16,
  XBOX_CODE_CATALOG = 17
};

struct VersionPair {
  uint16_t major{0};
  uint16_t minor{0};
  template <typename T> void Update(T major_, T minor_) {
    major = bela::swaple(major_);
    minor = bela::swaple(minor_);
  }
  std::wstring Str() const { return bela::StringCat(major, L".", minor); }
};

struct Attributes {
  std::wstring clrmsg;
  std::vector<std::wstring> depends; // depends dll
  std::vector<std::wstring> delays;  // delay load library
  VersionPair osver;
  VersionPair linkver;
  VersionPair imagever;
  Machine machine;
  Subsystem subsystem;
  uint16_t characteristics{0};
  uint16_t dllcharacteristics{0};
  bool IsConsole() const { return subsystem == Subsystem::CUI; }
  bool IsDLL() const {
    constexpr uint16_t imagefiledll = 0x2000;
    return (characteristics & imagefiledll) != 0;
  }
};
std::optional<Attributes> Expose(std::wstring_view file, bela::error_code &ec);

// https://docs.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfoexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-getfileversioninfosizeexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-verqueryvaluew
// version.lib
struct VersionInfo {
  std::wstring CompanyName;
  std::wstring FileDescription;
  std::wstring FileVersion;
  std::wstring InternalName;
  std::wstring LegalCopyright;
  std::wstring OriginalFileName;
  std::wstring ProductName;
  std::wstring ProductVersion;
  std::wstring Comments;
  std::wstring LegalTrademarks;
  std::wstring PrivateBuild;
  std::wstring SpecialBuild;
};

std::optional<VersionInfo> ExposeVersion(std::wstring_view file, bela::error_code &ec);

} // namespace bela::pe

#endif