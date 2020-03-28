//
#ifndef BELA_FNMATCH_HPP
#define BELA_FNMATCH_HPP
#include <string_view>

namespace bela {
namespace fnmatch {
[[maybe_unused]] constexpr int PathName = 0x1;
[[maybe_unused]] constexpr int NoEscape = 0x2;
[[maybe_unused]] constexpr int Period = 0x4;
[[maybe_unused]] constexpr int LeadingDir = 0x8;
[[maybe_unused]] constexpr int CaseFold = 0x10;
[[maybe_unused]] constexpr int FileName = PathName;
} // namespace fnmatch
// POSIX fnmatch impl see http://man7.org/linux/man-pages/man3/fnmatch.3.html
bool FnMatch(std::u16string_view pattern, std::u16string_view text,
             int flags = 0);
bool FnMatch(std::wstring_view pattern, std::wstring_view text, int flags = 0);
} // namespace bela

#endif