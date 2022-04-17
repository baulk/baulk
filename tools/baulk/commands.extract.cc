#include "baulk.hpp"
#include <bela/terminal.hpp>
#include <baulk/archive.hpp>
#include "extractor.hpp"
#include "commands.hpp"

namespace baulk::commands {

void usage_extract() {
  bela::FPrintF(stderr, LR"(Usage: baulk extract [archive] [destination]
Extract files from archive. alias 'e'.

Supported formats:
  zip (family) archive, supported methods: deflate, deflate64, zstd, bzip2, xz, lzma, Ppmd.
  tar, tar.gz, tar.xz, tar.bz2, tar.xz, tar.zstd
  gz, xz, bzip2, zstd,
  msi
  self-extracting archive
  7z supported archive

Example:
  baulk extract curl-7.80.0.tar.gz
  baulk extract curl-7.80.0.tar.gz curl-dest
  baulk e curl-7.80.0.zip
  baulk e curl-7.80.0.zip curl-dest

)");
}

int cmd_extract(const argv_t &argv) {
  if (argv.empty()) {
    usage_extract();
    return 1;
  }
  return baulk::extract_command_unchecked(argv, baulk::extract_command_auto);
}

} // namespace baulk::commands