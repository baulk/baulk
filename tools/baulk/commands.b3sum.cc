//
#include <bela/stdwriter.hpp>
#include "commands.hpp"
#include "hash.hpp"
#include "fs.hpp"
namespace baulk::commands {
int cmd_b3sum(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk b3sum file ...\n");
    return 1;
  }
  bela::error_code ec;
  for (const auto a : argv) {
    auto hv = baulk::hash::FileHashBLAKE3(a, ec);
    if (!hv) {
      bela::FPrintF(stderr, L"File: %s cannot calculate blake3 hash %s\n",
                    ec.message);
      continue;
    }
    bela::FPrintF(stdout, L"%s %s\n", *hv, baulk::fs::FileName(a));
  }
  return 0;
}
} // namespace baulk::commands