#include <bela/terminal.hpp>
#include <bela/color.hpp>

int wmain() {
  //
  bela::color c1;
  bela::color c2;
  if (bela::color::Decode(L"#88FFFF", c1)) {
    bela::FPrintF(stderr, L"r %d g %d b %d\n", c1.r, c1.g, c1.b);
  }
  if (bela::color::Decode(L"#FFFF42FF", c2)) {
    bela::FPrintF(stderr, L"r %d g %d b %d\n", c2.r, c2.g, c2.b);
  }
  bela::color c(132, 122, 64);
  bela::FPrintF(stderr, L"Encode -> %s\n", c.Encode(true));
  return 0;
}