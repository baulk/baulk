///
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>
#include <bela/codecvt.hpp>
#include "ucwidth-wt.hpp"

void print2() {
  bela::FPrintF(stderr, L"[%20d]\n", -9999);
  bela::FPrintF(stderr, L"[%-20d]\n", -9999);
  bela::FPrintF(stderr, L"[%020d]\n", -9999);
  bela::FPrintF(stderr, L"[%-020d]\n", -9999);
  bela::FPrintF(stderr, L"[%20X]\n", -99991111);
  bela::FPrintF(stderr, L"[%-20x]\n", -99998888);
  bela::FPrintF(stderr, L"[%020X]\n", -999900000);
  bela::FPrintF(stderr, L"[%2X]\n", -999900000);
  bela::FPrintF(stderr, L"[%20s]\n", "abcdefgf");
  bela::FPrintF(stderr, L"[%-20s]\n", "abcdefgf");
  bela::FPrintF(stderr, L"[%020s]\n", "abcdefgf");
  bela::FPrintF(stderr, L"[%-020s]\n", "abcdefgf");
  bela::FPrintF(stderr, L"[%0-20s]\n", "abcdefgf");
  bela::FPrintF(stderr, L"[%0-20v]0-20v\n", "abcdefgf");
  printf("printf 0-20v[%0-20s]\n", "abcdefg");
  bela::FPrintF(stderr, L"[%20d]\n", -999900000);
  bela::FPrintF(stderr, L"[%2X]\n", -999900000);
  bela::FPrintF(stderr, L"[%020v]\n", true);
  bela::FPrintF(stderr, L"[%20v]\n", true);
  bela::FPrintF(stderr, L"[%-020v]\n", true);
  bela::FPrintF(stderr, L"[%020.4f]\n", 1992.85);
  bela::FPrintF(stderr, L"[%20.8f %s]\n", 1993.85, 1993.85);
  bela::FPrintF(stderr, L"[%020.8f]\n", -3.141592654);
  bela::FPrintF(stderr, L"[%v]\n", -3.141592654);
}

int wmain(int argc, wchar_t **argv) {
  constexpr std::string_view ux = "\xf0\x9f\x98\x81 UTF-8 text \xE3\x8D\xA4 --> \xF0\xA0\x83\xA3 \x41 "
                                  "\xE7\xA0\xB4 \xE6\x99\x93"; // force encode UTF-8
  constexpr std::wstring_view wx = L"Engine (\xD83D\xDEE0) 中国 \U0001F496 \x0041 \x7834 "
                                   L"\x6653 \xD840\xDCE3";
  constexpr std::wstring_view wx2 = L"Engine (🛠) 中国 💖 A 破 晓 𠃣";
  constexpr std::u8string_view u8x = u8"💙 中国 \U0001F496 你爱我我爱你，蜜雪冰城甜蜜蜜";
  constexpr std::u16string_view u16x = u"💙 中国 \U0001F496 你爱我我爱你，蜜雪冰城甜蜜蜜";
  constexpr auto iscpp20 = __cplusplus >= 202002L;
  constexpr auto u16len = bela::string_length(u16x);
  constexpr auto u8len = bela::string_length(u8x);
  constexpr auto wlen = bela::string_length(wx);
  constexpr auto w2len = bela::string_length(wx2);
  bela::FPrintF(stderr,
                L"Argc: %d Arg0: \x1b[32m%s\x1b[0m W: %s UTF-8: %s "
                L"__cplusplus: %d C++20: %b\n%s\n",
                argc, argv[0], wx, ux, __cplusplus, iscpp20, u8x);
  bela::FPrintF(stderr, L"StringLength: %d StringLength %d\n", bela::string_length(u8x), bela::string_length(u16x));
  constexpr char32_t em = 0x1F603;     // 😃 U+1F603
  constexpr char32_t sh = 0x1F496;     //  💖
  constexpr char32_t blueheart = U'💙'; //💙
  constexpr char32_t se = 0x1F92A;     //🤪
  constexpr char32_t em2 = U'中';
  constexpr char32_t hammerandwrench = 0x1F6E0;

  wchar_t buf0[4];
  char16_t buf1[4];
  char buf2[8];
  char8_t buf3[8];

  bela::FPrintF(stderr, L"encode_into %s [wchar_t], %s [char16_t], %s [char] %s [char8_t]\n",
                bela::encode_into(sh, buf0), bela::encode_into(sh, buf1), bela::encode_into(sh, buf2),
                bela::encode_into(sh, buf3));

  auto s = bela::StringCat(L"Look emoji -->", em, L" U+", bela::AlphaNum(bela::Hex(em)));
  bela::FPrintF(stderr, L"emoji %c %c %c %c %U %U %s P: %p\n", em, sh, blueheart, se, em, em2, s,
                reinterpret_cast<const void *>(&em));
  bela::FPrintF(stderr, L"Unicode %c Width: %d \u2600 %d 中 %d ©: %d [%c] %d [%c] %d \n", em, bela::rune_width(em),
                bela::rune_width(0x2600), bela::rune_width(L'中'), bela::rune_width(0xA9), 161, bela::rune_width(161),
                hammerandwrench, bela::rune_width(hammerandwrench));
  bela::FPrintF(stderr, L"Unicode2 %c Width: %d \u2600 %d 中 %d  ©: %d [%c] %d [%c] %d\n", em,
                bela::unicode::CalculateWidthInternal(em), bela::unicode::CalculateWidthInternal(0x2600),
                bela::unicode::CalculateWidthInternal(L'中'), bela::unicode::CalculateWidthInternal(0xA9), 161,
                bela::unicode::CalculateWidthInternal(161), hammerandwrench,
                bela::unicode::CalculateWidthInternal(hammerandwrench));
  bela::FPrintF(stderr, L"[%-10d]\n", argc);
  bela::FPrintF(stderr, L"[%10d]\n", argc);
  bela::FPrintF(stderr, L"[%010d]\n", argc);
  int n = -1999;
  bela::FPrintF(stderr, L"[%-10d]\n", n);
  bela::FPrintF(stderr, L"[%10d]\n", n);
  bela::FPrintF(stderr, L"[%010d]\n", n);
  bela::FPrintF(stderr, L"[%-60d]\n", n);
  bela::FPrintF(stderr, L"[%60d]\n", n);
  bela::FPrintF(stderr, L"[%060d]\n", n);
  int n2 = 2999;
  bela::FPrintF(stderr, L"[%-10d]\n", n2);
  bela::FPrintF(stderr, L"[%10d]\n", n2);
  bela::FPrintF(stderr, L"[%010d]\n", n2);
  double ddd = 000192.15777411;
  bela::FPrintF(stderr, L"[%020.7f]\n", ddd);
  bela::FPrintF(stderr, L"[%.7f]\n", ddd);
  bela::FPrintF(stderr, L"[%2.7f]\n", ddd);
  long xl = 18256444;
  bela::FPrintF(stderr, L"[%-16x]\n", xl);
  bela::FPrintF(stderr, L"[%016X]\n", xl);
  bela::FPrintF(stderr, L"[%16X]\n", xl);
  bela::FPrintF(stderr, L"[0x%08X]\n", xl);
  bela::FPrintF(stderr, L"pointer: [%p]\n", (void *)argv);
  bela::FPrintF(stderr, L"StringWidth %d\n",
                bela::string_width<wchar_t>(LR"(cmake-3.20.5-windows-x86_64\share\vim\vimfiles\syntax\cmake.vim)"));
  bela::FPrintF(stderr, L"StringWidth %d\n",
                bela::string_width<char>(R"(cmake-3.20.5-windows-x86_64\share\vim\vimfiles\syntax\cmake.vim)"));
  print2();
  return 0;
}
