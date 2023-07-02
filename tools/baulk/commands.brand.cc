//
#include <bela/terminal.hpp>
#include <baulk/brand.hpp>
#include "commands.hpp"

namespace baulk::commands {
void usage_brand() {
  bela::FPrintF(stderr, L"Usage: baulk brand\n"); //
}

int cmd_brand(const argv_t & /*unused*/) {
  auto result = CoInitializeSecurity(NULL,
                                     -1,                          // COM authentication
                                     NULL,                        // Authentication services
                                     NULL,                        // Reserved
                                     RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
                                     RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
                                     NULL,                        // Authentication info
                                     EOAC_NONE,                   // Additional capabilities
                                     NULL                         // Reserved
  );
  if (FAILED(result)) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"baulk brand: CoInitializeSecurity error: %s\n", ec);
    return 1;
  }
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