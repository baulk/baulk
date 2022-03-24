///
#ifndef BAULK_DECOMPRESS_HPP
#define BAULK_DECOMPRESS_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/archive/format.hpp>
#include <filesystem>

namespace baulk {
bool extract_exe(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool extract_msi(std::wstring_view msi, std::wstring_view dest, bela::error_code &ec);
bool extract_zip(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool extract_7z(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool extract_tar(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool extract_auto(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool extract_tar(const bela::io::FD &fd, int64_t offset, baulk::archive::file_format_t afmt, std::wstring_view src,
                 std::wstring_view dest, bela::error_code &ec);
bool extract_auto_with_mode(std::wstring_view src, std::wstring_view dest, bool bucket_mode, bela::error_code &ec);
//
bool make_flattened(std::wstring_view path);
bool make_flattened_msi(std::wstring_view path);
bool make_flattened_exe(std::wstring_view path);
inline bool make_flattened_none(std::wstring_view path) { return true; }

struct extract_handler_t {
  std::wstring_view extension;
  decltype(&extract_exe) extract;
  decltype(&make_flattened) regularize;
};

inline std::optional<extract_handler_t> LookupHandler(std::wstring_view ext) {
  static constexpr extract_handler_t hs[] = {
      {L"exe", extract_exe, make_flattened_exe},    // exe
      {L"msi", extract_msi, make_flattened_msi},    // msi
      {L"zip", extract_zip, make_flattened},        // zip
      {L"7z", extract_7z, make_flattened},          // 7z
      {L"tar", extract_tar, make_flattened},        // tar
      {L"auto", extract_auto, make_flattened_none}, // auto detect
  };
  for (const auto &h : hs) {
    if (h.extension == ext) {
      return std::make_optional(h);
    }
  }
  return std::nullopt;
}
} // namespace baulk

#endif
