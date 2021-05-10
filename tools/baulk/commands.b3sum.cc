//
#include <bela/terminal.hpp>
#include "commands.hpp"
#include "hash.hpp"
#include "fs.hpp"

namespace baulk::commands {

void usage_b3sum() {
  bela::FPrintF(stderr, LR"(Usage: baulk b3sum [file] ...
Print BLAKE3 (256-bit) checksums.

Example:
  baulk b3sum baulk.zip

)");
}

int cmd_b3sum(const argv_t &argv) {
  if (argv.empty()) {
    usage_b3sum();
    return 1;
  }
  bela::error_code ec;
  for (const auto a : argv) {
    auto hv = baulk::hash::FileHash(a, baulk::hash::hash_t::BLAKE3, ec);
    if (!hv) {
      bela::FPrintF(stderr, L"File: %s cannot calculate blake3 checksum: \x1b[31m%s\x1b[0m\n", a, ec.message);
      continue;
    }
    bela::FPrintF(stdout, L"%s %s\n", *hv, baulk::fs::FileName(a));
  }
  return 0;
}
} // namespace baulk::commands