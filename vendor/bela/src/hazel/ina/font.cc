////////////// FONT resolve
#include "hazelinc.hpp"

namespace hazel::internal {

inline bool IsEot(bela::bytes_view bv) {
  return bv.size() > 35 && bv[34] == 0x4C && bv[35] == 0x50 &&
         ((bv[8] == 0x02 && bv[9] == 0x00 && bv[10] == 0x01) || (bv[8] == 0x01 && bv[9] == 0x00 && bv[10] == 0x00) ||
          (bv[8] == 0x02 && bv[9] == 0x00 && bv[10] == 0x02));
}

status_t LookupFonts(bela::bytes_view bv, hazel_result &hr) {
  switch (bv[0]) {
  case 0x00:
    if (bv.size() > 4 && bv[1] == 0x01 && bv[2] == 0x00 && bv[3] == 0x00 && bv[4] == 0x00) {
      // https://en.wikipedia.org/wiki/TrueType
      hr.assign(types::ttf, L"TrueType Font");
      return Found;
    }
    break;
  case 0x4F:
    if (bv.size() > 4 && bv[1] == 0x54 && bv[2] == 0x54 && bv[3] == 0x4F && bv[4] == 0x00) {
      // https://en.wikipedia.org/wiki/OpenType
      hr.assign(types::otf, L"OpenType Font");
      return Found;
    }
    break;
  case 0x77:
    if (bv.size() <= 7) {
      break;
    }
    if (bv[1] == 0x4F && bv[2] == 0x46 && bv[3] == 0x46 && bv[4] == 0x00 && bv[5] == 0x01 && bv[6] == 0x00 &&
        bv[7] == 0x00) {
      // https://en.wikipedia.org/wiki/Web_Open_Font_Format
      hr.assign(types::woff, L"Web Open Font Format");
      return Found;
    }
    if (bv[1] == 0x4F && bv[2] == 0x46 && bv[3] == 0x32 && bv[4] == 0x00 && bv[5] == 0x01 && bv[6] == 0x00 &&
        bv[7] == 0x00) {
      // https://en.wikipedia.org/wiki/Web_Open_Font_Format
      hr.assign(types::woff2, L"Web Open Font Format 2.0");
      return Found;
    }
    break;
  default:
    break;
  }
  if (IsEot(bv)) {
    hr.assign(types::eot, L"Embedded OpenType (EOT) fonts");
    return Found;
  }
  return None;
}
} // namespace hazel::internal