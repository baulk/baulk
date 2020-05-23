//
#include <bela/terminal.hpp>
#include "commands.hpp"
#include "hash.hpp"
#include "fs.hpp"

namespace baulk::commands {
int cmd_sha256sum(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk sha256sum file ...\n");
    return 1;
  }
  bela::error_code ec;
  for (const auto a : argv) {
    auto hv = baulk::hash::FileHashSHA256(a, ec);
    if (!hv) {
      bela::FPrintF(stderr, L"File: %s cannot calculate sha256 hash %s\n",
                    ec.message);
      continue;
    }
    bela::FPrintF(stdout, L"%s %s\n", *hv, baulk::fs::FileName(a));
  }
  return 0;
}
} // namespace baulk::commands