//
#include <bela/terminal.hpp>
#include <baulk/brand.hpp>
#include "commands.hpp"

namespace baulk::commands {
void usage_brand() {
  bela::FPrintF(stderr, L"Usage: baulk brand\n"); //
}

int cmd_brand(const argv_t &) {
  baulk::brand::Detector detector;
  bela::error_code ec;
  if (!detector.Execute(ec)) {
    bela::FPrintF(stderr, L"baulk brand: detect os information: %s\n", ec);
    return 1;
  }
  baulk::brand::Render render;
  detector.Swap(render);
  auto buffer = render.Draw();
  bela::FPrintF(stderr, L"%s\n", buffer);
  return 0;
}
} // namespace baulk::commands