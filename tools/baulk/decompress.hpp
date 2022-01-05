///
#ifndef BAULK_DECOMPRESS_HPP
#define BAULK_DECOMPRESS_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/archive/format.hpp>

namespace baulk {
namespace standard {
bool Regularize(std::wstring_view path);
}
namespace exe {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
bool Regularize(std::wstring_view path);
} // namespace exe
namespace msi {

bool Decompress(std::wstring_view msi, std::wstring_view dest, bela::error_code &ec);
bool Regularize(std::wstring_view path);
} // namespace msi
namespace zip {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
} // namespace zip
namespace sevenzip {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
} // namespace sevenzip
namespace tar {
bool TarExtract(const bela::io::FD &fd, int64_t offset, baulk::archive::file_format_t afmt, std::wstring_view src,
                std::wstring_view dest, bela::error_code &ec);
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
} // namespace tar

namespace smart {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec);
inline bool Regularize(std::wstring_view path) { return true; }
} // namespace smart

struct decompress_handler_t {
  std::wstring_view extension;
  decltype(&exe::Decompress) decompress;
  decltype(&exe::Regularize) regularize;
};

inline std::optional<decompress_handler_t> LookupHandler(std::wstring_view ext) {
  static constexpr decompress_handler_t hs[] = {
      {L"exe", exe::Decompress, exe::Regularize},          // exe
      {L"msi", msi::Decompress, msi::Regularize},          // msi
      {L"zip", zip::Decompress, standard::Regularize},     // zip
      {L"7z", sevenzip::Decompress, standard::Regularize}, // 7z
      {L"tar", tar::Decompress, standard::Regularize},     // tar
      {L"auto", smart::Decompress, smart::Regularize},     // auto detect
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
