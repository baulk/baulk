#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/__unicode/unicode-width.hpp>

namespace mock {
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
} // namespace mock

int wmain() {
  constexpr char32_t em = 0x1F603;     // 😃 U+1F603
  constexpr char32_t sh = 0x1F496;     //  💖
  constexpr char32_t blueheart = U'💙'; //💙
  constexpr char32_t se = 0x1F92A;     //🤪
  constexpr char32_t em2 = U'中';
  constexpr char32_t hammerandwrench = 0x1F6E0;
  bela::FPrintF(stderr, L"Unicode %c Width: %d \u2600 %d 中 %d ©: %d [%c] %d [%c] %d \n", em, mock::rune_width(em),
                mock::rune_width(0x2600), mock::rune_width(L'中'), mock::rune_width(0xA9), 161, mock::rune_width(161),
                hammerandwrench, mock::rune_width(hammerandwrench));
  constexpr char32_t codes[] = {0x1F603, 0x1F496, U'💙', 0x1F92A, 0x1FA75, 0x1FABB, 0x1FAF6, U'✅'};
  for (const auto c : codes) {
    bela::FPrintF(stderr, L"%v width: %d\n", c, mock::rune_width(c));
  }
}