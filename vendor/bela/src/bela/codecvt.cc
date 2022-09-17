//////////
// see:
// https://github.com/llvm-mirror/llvm/blob/master/lib/Support/ConvertUTF.cpp
//
#include <bela/codecvt.hpp>
#include <bela/types.hpp>
#include <bela/__unicode/unicode-width.hpp>

namespace bela {

bool bisearch(char32_t rune, const bela::unicode::interval *table, size_t max) {
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

size_t rune_width(char32_t rune) {
  // control characters we return 0;
  if (rune == 0 || rune > 0x10FFFF || rune < 32 || (rune >= 0x7F && rune < 0xa0)) {
    return 0;
  }
  if (bisearch(rune, bela::unicode::zero_width, std::size(bela::unicode::zero_width) - 1)) {
    return 0;
  }
  if (bisearch(rune, bela::unicode::double_width, std::size(bela::unicode::double_width) - 1)) {
    return 2;
  }
  return 1;
}

} // namespace bela
