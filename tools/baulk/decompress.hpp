///
#ifndef BAULK_DECOMPRESS_HPP
#define BAULK_DECOMPRESS_HPP
#include <bela/base.hpp>

namespace baulk {
namespace standard {
bool Regularize(std::wstring_view path);
}
namespace exe {
bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec);
bool Regularize(std::wstring_view path);
} // namespace exe
namespace msi {

bool Decompress(std::wstring_view msi, std::wstring_view outdir,
                bela::error_code &ec);
bool Regularize(std::wstring_view path);
} // namespace msi
namespace zip {
bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec);
} // namespace zip
namespace sevenzip {
bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec);
} // namespace sevenzip

struct decompress_handler_t {
  std::wstring_view extension;
  decltype(&exe::Decompress) decompress;
  decltype(&exe::Regularize) regularize;
};

inline std::optional<decompress_handler_t>
LookupHandler(std::wstring_view ext) {
  static constexpr decompress_handler_t hs[] = {
      {L"exe", exe::Decompress, exe::Regularize},
      {L"msi", msi::Decompress, msi::Regularize},
      {L"zip", zip::Decompress, standard::Regularize},
      {L"7z", sevenzip::Decompress, standard::Regularize}
      //
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