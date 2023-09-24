#include <bela/codecvt.hpp>
#include <bela/path.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/str_split.hpp>
#include <baulk/archive.hpp>
#include <compact_enc_det/compact_enc_det.h>
#include <filesystem>

namespace baulk::archive {
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

inline std::wstring encode_from_codepage(std::string_view name, int codePage = CP_ACP) {
  auto sz = MultiByteToWideChar(codePage, 0, name.data(), (int)name.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  MultiByteToWideChar(codePage, 0, name.data(), (int)name.size(), output.data(), sz);
  return output;
}

inline std::wstring encode_into_native(std::string_view filename, bool always_utf8) {
  if (always_utf8) {
    return bela::encode_into<char, wchar_t>(filename);
  }
  bool is_reliable = false;
  int bytes_consumed = 0;
  auto e = CompactEncDet::DetectEncoding(filename.data(), static_cast<int>(filename.size()), nullptr, nullptr, nullptr,
                                         UNKNOWN_ENCODING, UNKNOWN_LANGUAGE, CompactEncDet::WEB_CORPUS, false,
                                         &bytes_consumed, &is_reliable);
  return encode_from_codepage(filename, codePageSearch(e));
}

std::wstring EncodeToNativePath(std::string_view filename, bool always_utf8) {
  return encode_into_native(filename, always_utf8);
}

constexpr bool IsDangerousPath(std::wstring_view p) {
  constexpr std::wstring_view dangerousPaths[] = {L":$i30:$bitmap", L"$mft"};
  for (const auto d : dangerousPaths) {
    if (p.ends_with(d)) {
      return true;
    }
  }
  return false;
}

std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view child_path, bool always_utf8) {
  auto fileName = encode_into_native(child_path, always_utf8);
  auto path = bela::PathCat(root, fileName);
  if (IsDangerousPath(path)) {
    return std::nullopt; // Windows BUG
  }
  if (path.size() <= root.size()) {
    return std::nullopt;
  }
  if (!path.starts_with(root)) {
    return std::nullopt;
  }
  if (!root.ends_with('/') && !bela::IsPathSeparator(path[root.size()])) {
    return std::nullopt;
  }
  return std::make_optional(std::move(path));
}

inline bool is_harmful_path(std::string_view child_path) {
  const std::string_view dot = ".";
  const std::string_view dotdot = "..";
  std::vector<std::string_view> paths =
      bela::narrow::StrSplit(child_path, bela::narrow::ByAnyChar("\\/"), bela::narrow::SkipEmpty());
  int entries = 0;
  for (auto p : paths) {
    if (p == dot) {
      continue;
    }
    if (p != dotdot) {
      entries++;
      continue;
    }
    entries--;
    if (entries < 0) {
      return true;
    }
  }
  return entries <= 0;
}
bool IsHarmfulPath(std::string_view child_path) { return is_harmful_path(child_path); }

std::optional<std::filesystem::path> JoinSanitizeFsPath(const std::filesystem::path &root, std::string_view child_path,
                                                        bool always_utf8, std::wstring &encoded_path) {
  if (is_harmful_path(child_path)) {
    return std::nullopt;
  }
  encoded_path = encode_into_native(child_path, always_utf8);
  return std::make_optional(root / encoded_path);
}

} // namespace baulk::archive