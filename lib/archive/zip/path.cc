///
#include "zipinternal.hpp"
#include <bela/path.hpp>
#include <compact_enc_det/compact_enc_det.h>

namespace baulk::archive::zip {
uint32_t codePageSearch(Encoding e) {
  static constexpr const struct {
    Encoding e;
    uint32_t codePage;
  } codePages[] = {
      {CHINESE_GB, 936},         //
      {CHINESE_BIG5, 950},       //
      {JAPANESE_EUC_JP, 20932},  //
      {JAPANESE_SHIFT_JIS, 932}, //
      {KOREAN_EUC_KR, 51949},    //
      {GB18030, 54936},          //
      {HZ_GB_2312, 52936},       //
  };
  for (const auto c : codePages) {
    if (c.e == e) {
      return c.codePage;
    }
  }
  return CP_ACP;
}

// https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
std::wstring FileNameRecoding(std::string_view name) {
  bool is_reliable = false;
  int bytes_consumed = 0;
  auto e = CompactEncDet::DetectEncoding(name.data(), static_cast<int>(name.size()), nullptr, nullptr, nullptr,
                                         UNKNOWN_ENCODING, UNKNOWN_LANGUAGE, CompactEncDet::WEB_CORPUS, false,
                                         &bytes_consumed, &is_reliable);
  const auto cp = codePageSearch(e);
  auto sz = MultiByteToWideChar(cp, 0, name.data(), (int)name.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  MultiByteToWideChar(cp, 0, name.data(), (int)name.size(), output.data(), sz);
  return output;
}

inline std::wstring PathConvert(const File &file) {
  if (file.IsFileNameUTF8()) {
    return bela::ToWide(file.name);
  }
  return FileNameRecoding(file.name);
}

std::optional<std::wstring> PathCat(std::wstring_view root, const File &file) {
  auto filename = PathConvert(file);
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