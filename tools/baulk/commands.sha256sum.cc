//
#include <bela/terminal.hpp>
#include <baulk/hash.hpp>
#include <baulk/fs.hpp>
#include "commands.hpp"

namespace baulk::commands {
void usage_sha256sum() {
  bela::FPrintF(stderr, LR"(Usage: baulk sha256sum [file] ...
Print SHA256 (256-bit) checksums.

Example:
  baulk sha256sum baulk.zip

)");
}
int cmd_sha256sum(const argv_t &argv) {
  if (argv.empty()) {
    usage_sha256sum();
    return 1;
  }
  bela::error_code ec;
  for (const auto a : argv) {
    auto hv = baulk::hash::FileHash(a, baulk::hash::hash_t::SHA256, ec);
    if (!hv) {
      bela::FPrintF(stderr, L"File: '%s' cannot calculate sha256 checksum: \x1b[31m%s\x1b[0m\n", a, ec.message);
      continue;
    }
    bela::FPrintF(stdout, L"%s %s\n", *hv, baulk::fs::FileName(a));
  }
  return 0;
}
} // namespace baulk::commands