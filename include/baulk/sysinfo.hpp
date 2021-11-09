#ifndef BAULK_SYSINFO_HPP
#define BAULK_SYSINFO_HPP
#include <bela/base.hpp>

namespace baulk::osinfo {

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