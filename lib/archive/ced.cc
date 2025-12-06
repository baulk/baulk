#include <bela/codecvt.hpp>
#include <bela/path.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/str_split.hpp>
#include <bela/str_replace.hpp>
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
      {.e = CHINESE_GB, .codePage = 936}, // ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
      {.e = CHINESE_BIG5,
       .codePage = 950}, // ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
      {.e = CHINESE_BIG5_CP950, .codePage = 950}, //
      {.e = GBK, .codePage = 936},                // GBK use 936?
      {.e = CHINESE_EUC_DEC, .codePage = 51936},  // EUC Simplified Chinese; Chinese Simplified (EUC)
      {.e = GB18030,
       .codePage = 54936}, // Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
      {.e = HZ_GB_2312, .codePage = 52936},       // HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
      {.e = JAPANESE_EUC_JP, .codePage = 20932},  // Japanese (JIS 0208-1990 and 0212-1990)
      {.e = JAPANESE_SHIFT_JIS, .codePage = 932}, // ANSI/OEM Japanese; Japanese (Shift-JIS)
      {.e = JAPANESE_CP932, .codePage = 932},     //
      {.e = KOREAN_EUC_KR, .codePage = 51949},    //	EUC Korean
      {.e = ISO_8859_1, .codePage = 28591},       // ISO 8859-1 Latin 1; Western European (ISO)
      {.e = ISO_8859_2, .codePage = 28592},       //	ISO 8859-2 Central European; Central European (ISO)
      {.e = ISO_8859_3, .codePage = 28593},       // in BasisTech but not in Teragram
      {.e = ISO_8859_4, .codePage = 28594},       // Teragram Latin4
      {.e = ISO_8859_5, .codePage = 28595},       // Teragram ISO-8859-5
      {.e = ISO_8859_6, .codePage = 28596},       // Teragram Arabic
      {.e = ISO_8859_7, .codePage = 28597},       // Teragram Greek
      {.e = ISO_8859_8, .codePage = 28598},       // Teragram Hebrew
      {.e = ISO_8859_9, .codePage = 28599},       // in BasisTech but not in Teragram
      {.e = RUSSIAN_CP1251, .codePage = 1251},    //
      {.e = MSFT_CP1252, .codePage = 1252},       //
      {.e = RUSSIAN_KOI8_R, .codePage = 20866},   // Russian (KOI8-R); Cyrillic (KOI8-R)
      {.e = ISO_8859_15, .codePage = 28605},      // ISO 8859-15 Latin 9
      {.e = MSFT_CP1254, .codePage = 1254},       //
      {.e = MSFT_CP1257, .codePage = 1257},       //
      {.e = MSFT_CP874, .codePage = 847},         //
      {.e = MSFT_CP1256, .codePage = 1256},       //
      {.e = MSFT_CP1255, .codePage = 1255},       //
      {.e = ISO_8859_8_I, .codePage = 38598},     // ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
      {.e = HEBREW_VISUAL, .codePage = 28598},    // ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
      {.e = CZECH_CP852, .codePage = 852},        //
      {.e = ISO_8859_13, .codePage = 28603},      // ISO 8859-13 Estonian
      {.e = ISO_2022_KR, .codePage = 50225},      // ISO 2022 Korean
      {.e = ISO_2022_CN, .codePage = 50227}       // ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
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
  constexpr std::wstring_view excludeChars = L"\r\n<>:\"|*?";
  if (encoded_path.find_first_of(excludeChars) != std::wstring::npos) {
    bela::StrReplaceAll(
        {
            // replace --> < --> _
            {L"\r", L"-"},
            {L"\n", L"_"},
            {L"<", L"_"},
            {L">", L"_"},
            {L":", L"_"},
            {L"\"", L"_"},
            {L"|", L"_"},
            {L"|", L"_"},
            {L"?", L"_"} //
        },
        &encoded_path);
  }
  return std::make_optional(root / encoded_path);
}

} // namespace baulk::archive