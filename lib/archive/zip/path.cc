///
#include "zipinternal.hpp"
#include <bela/path.hpp>
#include <compact_enc_det/compact_enc_det.h>

namespace baulk::archive::zip {
// https://codereview.chromium.org/2081653007/
// https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
uint32_t codePageSearch(Encoding e) {
  static constexpr const struct {
    Encoding e;
    uint32_t codePage;
  } codePages[] = {
      {CHINESE_GB, 936},   // ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
      {CHINESE_BIG5, 950}, // ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
      {CHINESE_BIG5_CP950, 950}, //
      {GBK, 936},                // GBK use 936?
      {CHINESE_EUC_DEC, 51936},  // EUC Simplified Chinese; Chinese Simplified (EUC)
      {GB18030, 54936},    // Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
      {HZ_GB_2312, 52936}, // HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
      {JAPANESE_EUC_JP, 20932},  // Japanese (JIS 0208-1990 and 0212-1990)
      {JAPANESE_SHIFT_JIS, 932}, // ANSI/OEM Japanese; Japanese (Shift-JIS)
      {JAPANESE_CP932, 932},     //
      {KOREAN_EUC_KR, 51949},    //	EUC Korean
      {ISO_8859_1, 28591},       // ISO 8859-1 Latin 1; Western European (ISO)
      {ISO_8859_2, 28592},       //	ISO 8859-2 Central European; Central European (ISO)
      {ISO_8859_3, 28593},       // in BasisTech but not in Teragram
      {ISO_8859_4, 28594},       // Teragram Latin4
      {ISO_8859_5, 28595},       // Teragram ISO-8859-5
      {ISO_8859_6, 28596},       // Teragram Arabic
      {ISO_8859_7, 28597},       // Teragram Greek
      {ISO_8859_8, 28598},       // Teragram Hebrew
      {ISO_8859_9, 28599},       // in BasisTech but not in Teragram
      {RUSSIAN_CP1251, 1251},    //
      {MSFT_CP1252, 1252},       //
      {RUSSIAN_KOI8_R, 20866},   // Russian (KOI8-R); Cyrillic (KOI8-R)
      {ISO_8859_15, 28605},      // ISO 8859-15 Latin 9
      {MSFT_CP1254, 1254},       //
      {MSFT_CP1257, 1257},       //
      {MSFT_CP874, 847},         //
      {MSFT_CP1256, 1256},       //
      {MSFT_CP1255, 1255},       //
      {ISO_8859_8_I, 38598},     // ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
      {HEBREW_VISUAL, 28598},    // ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
      {CZECH_CP852, 852},        //
      {ISO_8859_13, 28603},      // ISO 8859-13 Estonian
      {ISO_2022_KR, 50225},      // ISO 2022 Korean
      {ISO_2022_CN, 50227}       // ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
  };
  for (const auto c : codePages) {
    if (c.e == e) {
      return c.codePage;
    }
  }
  return CP_ACP;
}

inline std::wstring fromcodePage(std::string_view name, int codePage = CP_ACP) {
  auto sz = MultiByteToWideChar(codePage, 0, name.data(), (int)name.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  MultiByteToWideChar(codePage, 0, name.data(), (int)name.size(), output.data(), sz);
  return output;
}

inline std::wstring PathConvert(const File &file, bool autocvt) {
  if (file.IsFileNameUTF8()) {
    return bela::ToWide(file.name);
  }
  if (!autocvt) {
    return fromcodePage(file.name);
  }
  bool is_reliable = false;
  int bytes_consumed = 0;
  auto e = CompactEncDet::DetectEncoding(file.name.data(), static_cast<int>(file.name.size()), nullptr, nullptr,
                                         nullptr, UNKNOWN_ENCODING, UNKNOWN_LANGUAGE, CompactEncDet::WEB_CORPUS, false,
                                         &bytes_consumed, &is_reliable);
  return fromcodePage(file.name, codePageSearch(e));
}

std::optional<std::wstring> PathCat(std::wstring_view root, const File &file, bool autocvt) {
  auto filename = PathConvert(file, autocvt);
  auto path = bela::PathCat(root, filename);
  if (path == L"." || !path.starts_with(root)) {
    return std::nullopt;
  }
  if (!root.ends_with('/') && !bela::IsPathSeparator(path[root.size()])) {
    return std::nullopt;
  }
  return std::make_optional(std::move(path));
}
} // namespace baulk::archive::zip