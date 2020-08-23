////
#include <iterator>
#include <bela/ucwidth.hpp>
#include "runewidth_table.hpp"

namespace bela {
constexpr const bela::runewidth::interval nonprint[] = {
    {0x0000, 0x001F}, {0x007F, 0x009F}, {0x00AD, 0x00AD}, {0x070F, 0x070F}, {0x180B, 0x180E}, {0x200B, 0x200F},
    {0x2028, 0x202E}, {0x206A, 0x206F}, {0xD800, 0xDFFF}, {0xFEFF, 0xFEFF}, {0xFFF9, 0xFFFB}, {0xFFFE, 0xFFFF},
};
bool bisearch(char32_t rune, const bela::runewidth::interval *table, size_t max) {
  size_t min = 0;
  size_t mid;
  if (rune < table[0].first || rune > table[max].last) {
    return false;
  }
  while (max >= min) {
    mid = (min + max) / 2;
    if (rune > table[mid].last) {
      min = mid + 1;
      continue;
    }
    if (rune < table[mid].first) {
      max = mid - 1;
      continue;
    }
    return true;
  }
  return false;
}

// CalculateLength calculate unicode codepoint display width
size_t CalculateWidth(char32_t rune) {
  // control characters we return 0;
  if (rune == 0 || rune < 32 || (rune >= 0x7F && rune < 0xa0)) {
    return 0;
  }
  if (bisearch(rune, bela::nonprint, std::size(bela::nonprint) - 1)) {
    return 0;
  }
  if (bisearch(rune, bela::runewidth::notassigned, std::size(bela::runewidth::notassigned) - 1)) {
    return 0;
  }
  if (bisearch(rune, bela::runewidth::combining, std::size(bela::runewidth::combining) - 1)) {
    return 0;
  }
  if (bisearch(rune, bela::runewidth::ambiguous, std::size(bela::runewidth::ambiguous) - 1)) {
    return 2;
  }
  if (bisearch(rune, bela::runewidth::doublewidth, std::size(bela::runewidth::doublewidth) - 1)) {
    return 2;
  }
  if (bisearch(rune, bela::runewidth::emoji, std::size(bela::runewidth::emoji) - 1)) {
    return 2;
  }
  return 1;
}

} // namespace bela
