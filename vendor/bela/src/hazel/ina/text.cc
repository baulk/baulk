////////////////
#include "hazelinc.hpp"
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

namespace hazel::internal {
// check text details

bool buffer_is_binary(bela::MemView mv) {
  auto size = (std::min)(mv.size(), size_t(0x8000));
  if (memchr(mv.data(), 0, size) != nullptr) {
    return true;
  }
  return false;
}

/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

// Thanks
// https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2018/05/08/checkutf8.c
static const uint8_t utf8d[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 00..1f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 20..3f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 40..5f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 60..7f
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   9,   9,   9,   9,   9,   9,   //
    9,   9,   9,   9,   9,   9,   9,   9,   9,   9,        // 80..9f
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   //
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   //
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,        // a0..bf
    8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   //
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   //
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,        // c0..df
    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, //
    0x3, 0x3, 0x4, 0x3, 0x3,                               // e0..ef
    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, //
    0x8, 0x8, 0x8, 0x8, 0x8                                // f0..ff
};

static const uint8_t utf8d_transition[] = {
    0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, //
    0x6, 0x1, 0x1, 0x1, 0x1,                               // s0..s0
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,   //
    1,   0,   1,   0,   1,   1,   1,   1,   1,   1,        // s1..s2
    1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   2,   1,   1,   1,   1,   1,   1,   1,   1,        // s3..s4
    1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   3,   1,   3,   1,   1,   1,   1,   1,   1,        // s5..s6
    1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   //
    1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,        // s7..s8
};

static inline uint32_t updatestate(uint32_t *state, uint32_t byte) {
  uint32_t type = utf8d[byte];
  *state = utf8d_transition[16 * *state + type];
  return *state;
}

bool validate_utf8(const char *c, size_t len) {
  const unsigned char *cu = (const unsigned char *)c;
  uint32_t state = 0;
  for (size_t i = 0; i < len; i++) {
    uint32_t byteval = (uint32_t)cu[i];
    if (updatestate(&state, byteval) == UTF8_REJECT) {
      return false;
    }
  }
  return true;
}

/*
00 00 FE FF	UTF-32, big-endian
FF FE 00 00	UTF-32, little-endian
FE FF	UTF-16, big-endian
FF FE	UTF-16, little-endian
EF BB BF	UTF-8
*/
status_t lookup_text(bela::MemView mv, hazel_result &hr) {
  //
  switch (mv[0]) {
  case 0x2B:
    if (mv.size() >= 3 && mv[1] == 0x2F && mv[2] == 0xbf) {
      // constexpr const byte_t utf7mgaic[]={0x2b,0x2f,0xbf};
      hr.assign(types::utf7, L"UTF-7 text");
    }
    break;
  case 0xEF: // UTF8 BOM 0xEF 0xBB 0xBF
    if (mv.size() >= 3 && mv[1] == 0xBB && mv[2] == 0xBF) {
      hr.assign(types::utf8bom, L"UTF-8 Unicode (with BOM) text");
      return Found;
    }
    break;
  case 0xFF: // UTF16LE 0xFF 0xFE
    if (mv.size() > 4 && mv[1] == 0xFE && mv[2] == 0 && mv[3] == 0) {
      hr.assign(types::utf32le, L"Little-endian UTF-32 Unicode text");
      return Found;
    }
    if (mv.size() >= 2 && mv[1] == 0xFE) {
      hr.assign(types::utf16le, L"Little-endian UTF-16 Unicode text");
      return Found;
    }

    break;
  case 0xFE: // UTF16BE 0xFE 0xFF
    if (mv.size() >= 2 && mv[1] == 0xFF) {
      hr.assign(types::utf16be, L"Big-endian UTF-16 Unicode text");
      return Found;
    }
    break;
    // FF FE 00 00
  case 0x0:
    if (mv.size() >= 4 && mv[1] == 0 && mv[2] == 0xFE && mv[3] == 0xFF) {
      hr.assign(types::utf32be, L"Big-endian UTF-32 Unicode text");
      return Found;
    }
    break;
  default:
    break;
  }
  return None;
}

//////// --------------> use chardet
status_t lookup_chardet(bela::MemView mv, hazel_result &hr) {
  if (buffer_is_binary(mv)) {
    hr.assign(types::none, L"Binary data");
    return Found;
  }
  hr.assign(types::utf8, L"UTF-8 Unicode text");
  return Found;
}

status_t LookupText(bela::MemView mv, hazel_result &hr) {
  if (lookup_text(mv, hr) != Found) {
    lookup_chardet(mv, hr);
  }
  // check text
  std::wstring shebangline;
  switch (hr.type()) {
  case types::utf8: {
    // Note that we may get truncated UTF-8 data
    auto line = mv.sv();
    auto pos = line.find_first_of("\r\n");
    if (pos != std::string_view::npos) {
      line = line.substr(0, pos);
    }
    shebangline = bela::ToWide(line);
  } break;
  case types::utf8bom: {
    // Note that we may get truncated UTF-8 data
    auto line = mv.submv(3).sv();
    auto pos = line.find_first_of("\r\n");
    if (pos != std::string_view::npos) {
      line = line.substr(0, pos);
    }
    shebangline = bela::ToWide(line);
  } break;
  case types::utf16le: {
    auto line = mv.submv(2).wsv();
    auto pos = line.find_first_of(L"\r\n");
    if (pos != std::wstring_view::npos) {
      line = line.substr(0, pos);
    }
    shebangline = line;
  } break;
  case types::utf16be: {
    auto besb = mv.submv(2).wsv();
    shebangline.resize(besb.size());
    for (size_t i = 0; i < besb.size(); i++) {
      shebangline[i] = static_cast<wchar_t>(bela::swap16(static_cast<uint16_t>(besb[i])));
    }
    auto pos = shebangline.find_first_of(L"\r\n");
    if (pos != std::wstring::npos) {
      shebangline.resize(pos);
    }
  } break;
  default:
    return Found;
  }

  if (hazel::internal::LookupShebang(shebangline, hr)) {
    return Found;
  }
  // shlbang
  return Found;
}

} // namespace hazel::internal
