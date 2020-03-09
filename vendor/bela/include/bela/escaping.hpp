// escape.hpp
#ifndef BELA_ESCAPING_HPP
#define BELA_ESCAPING_HPP
#include <cstddef>
#include <string>
#include <vector>
#include <string_view>

namespace bela {
bool CUnescape(std::wstring_view source, std::wstring *dest,
               std::wstring *error);

// Overload of `CUnescape()` with no error reporting.
inline bool CUnescape(std::wstring_view source, std::wstring *dest) {
  return CUnescape(source, dest, nullptr);
}
std::wstring CEscape(std::wstring_view src);
} // namespace bela

#endif
