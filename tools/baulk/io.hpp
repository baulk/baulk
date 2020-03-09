//
#ifndef BAULK_IO_HPP
#define BAULK_IO_HPP
#include <bela/base.hpp>

namespace baulk::io {
[[maybe_unused]] constexpr auto MaximumRead = 1024ull * 1024 * 8;
[[maybe_unused]] constexpr auto MaximumLineLength = 1024ull * 64;
bool ReadAll(std::wstring_view file, std::wstring &out, bela::error_code &ec,
             uint64_t maxsize = MaximumRead);
// readline
std::optional<std::wstring> ReadLine(std::wstring_view file,
                                     bela::error_code &ec,
                                     uint64_t maxline = MaximumLineLength);
} // namespace baulk::io

#endif