#include <bela/terminal.hpp>
#include <bela/color.hpp>

int wmain() {
  //
  if (auto c1 = bela::color::decode(L"#88FFFF"); c1 == bela::color(0x88, 0xff, 0xff)) {
    bela::FPrintF(stderr, L"#88FFFF: r %d g %d b %d: %s\n", c1.r, c1.g, c1.b, c1.encode());
  }
  if (auto c2 = bela::color::decode(L"#FFFF42FF"); c2 == bela::color(0xff, 0xff, 0x42, 0xff)) {
    bela::FPrintF(stderr, L"#FFFF42FF: r %d g %d b %d: %s\n", c2.r, c2.g, c2.b, c2.encode(true));
  }
  bela::color c(132, 122, 64);
  bela::FPrintF(stderr, L"Encode -> %s\n", c.encode<char16_t>(true));
  return 0;
}