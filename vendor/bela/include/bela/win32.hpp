#ifndef BELA_WIN32_HPP
#define BELA_WIN32_HPP
#include <bela/base.hpp>

namespace bela {

namespace windows {
namespace windows_internal {
struct version {
  std::uint32_t major{0};
  std::uint32_t minor{0};
  std::uint32_t build{0};
  std::uint32_t service_pack_major{0};
  std::uint32_t service_pack_minor{0};
  std::uint8_t product_type{VER_NT_WORKSTATION};
  constexpr version(std::uint32_t major_, std::uint32_t minor_, std::uint32_t build_,
                    std::uint32_t service_pack_major_ = 0, std::uint32_t service_pack_minor_ = 0,
                    std::uint8_t product_type_ = 0)
      : major{major_}, minor{minor_}, build{build_}, service_pack_major{service_pack_major_},
        service_pack_minor{service_pack_minor_}, product_type{product_type_} {}
  constexpr version() = default;
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
  constexpr version(const version &) = default;
  constexpr version(version &&) = default;
  ~version() = default;
  constexpr version &operator=(const version &) = default;
  constexpr version &operator=(version &&) = default;
  constexpr bool station() const noexcept { return product_type == VER_NT_WORKSTATION; }
  [[nodiscard]] constexpr int compare(const version &other) const noexcept {
    if (major != other.major) {
      return major - other.major;
    }
    if (minor != other.minor) {
      return minor - other.minor;
    }
    if (build != other.build) {
      return build - other.build;
    }
    if (service_pack_major != other.service_pack_major) {
      return service_pack_major - other.service_pack_major;
    }
    if (service_pack_minor != other.service_pack_minor) {
      return service_pack_minor - other.service_pack_minor;
    }
    return 0;
  }
};

constexpr bool operator==(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) == 0; }
constexpr bool operator!=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) != 0; }
constexpr bool operator>(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) > 0; }
constexpr bool operator>=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) >= 0; }
constexpr bool operator<(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) < 0; }
constexpr bool operator<=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) <= 0; }

} // namespace windows_internal
constexpr windows_internal::version win10_rs5(10, 0, 17763);
constexpr windows_internal::version win10_19h1(10, 0, 18362);
constexpr windows_internal::version win10_19h2(10, 0, 18363);
constexpr windows_internal::version win10_20h1(10, 0, 19041);
constexpr windows_internal::version win10_20h2(10, 0, 19042);
constexpr windows_internal::version win10_21h1(10, 0, 19043);
constexpr windows_internal::version win10_21h2(10, 0, 19044);
constexpr windows_internal::version win11_21h2(10, 0, 22000);
constexpr windows_internal::version server_2022(10, 0, 20348);

inline windows_internal::version version() {
  using ntstatus = LONG;
  constexpr auto status_success = ntstatus(0x00000000);
  ntstatus WINAPI RtlGetVersion(PRTL_OSVERSIONINFOW);
  auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) {
    return win10_20h1;
  }
  auto rtl_get_version = reinterpret_cast<decltype(RtlGetVersion) *>(GetProcAddress(ntdll, "RtlGetVersion"));
  if (rtl_get_version == nullptr) {
    return win10_20h1;
  }
  RTL_OSVERSIONINFOEXW rovi = {0};
  rovi.dwOSVersionInfoSize = sizeof(rovi);
  if (rtl_get_version(reinterpret_cast<PRTL_OSVERSIONINFOW>(&rovi)) != status_success) {
    return win10_20h1;
  }
  return windows_internal::version(rovi.dwMajorVersion, rovi.dwMinorVersion, rovi.dwBuildNumber, rovi.wServicePackMajor,
                                   rovi.wServicePackMinor, rovi.wProductType);
}
} // namespace windows
using windows_version = bela::windows::windows_internal::version;
} // namespace bela

#endif