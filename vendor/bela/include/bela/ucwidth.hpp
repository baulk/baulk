///
#ifndef BELA_UNICODE_WIDTH_HPP
#define BELA_UNICODE_WIDTH_HPP
#include <cstddef>

namespace bela {
// CalculateLength calculate unicode codepoint display width
size_t CalculateWidth(char32_t rune);
} // namespace bela

#endif
