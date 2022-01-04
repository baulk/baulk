#include "baulk.hpp"
#include <bela/terminal.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/extract.hpp>
#include "decompress.hpp"
#include "commands.hpp"

namespace baulk::commands {

void usage_extract() {
  bela::FPrintF(stderr, LR"(Usage: baulk extract [archive] [destination]
Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd

Example:
  baulk extract curl-7.80.0.tar.gz
  baulk extract curl-7.80.0.tar.gz curl-dest
  baulk e curl-7.80.0.tar.gz
  baulk e curl-7.80.0.tar.gz curl-dest

)");
}
using archive::file_format_t;

int zip_extract(bela::io::FD &fd, std::wstring_view dest, int64_t offset) {
  baulk::archive::ZipExtractor extractor(baulk::IsQuietMode);
  bela::error_code ec;
  if (!extractor.OpenReader(fd, dest, bela::SizeUnInitialized, offset, ec)) {
    bela::FPrintF(stderr, L"baulk extract zip error: %s\n", ec);
    return 1;
  }
  if (!extractor.Extract(ec)) {
    bela::FPrintF(stderr, L"baulk extract zip error: %s\n", ec);
    return 1;
  }
  return 0;
}

int cmd_extract(const argv_t &argv) {
  if (argv.empty()) {
    usage_extract();
    return 1;
  }
  auto arfile = argv[0];
  auto dest = argv.size() > 1 ? std::wstring(argv[1]) : baulk::archive::FileDestination(arfile);
  file_format_t afmt{file_format_t::none};
  int64_t baseOffest = 0;
  bela::error_code ec;
  auto fd = archive::OpenArchiveFile(arfile, baseOffest, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", arfile, ec);
    return 1;
  }
  DbgPrint(L"detect file format: %s", archive::FormatToMIME(afmt));
  switch (afmt) {
  case file_format_t::zip:
    return zip_extract(*fd, dest, baseOffest);
  case file_format_t::xz:
    [[fallthrough]];
  case file_format_t::zstd:
    [[fallthrough]];
  case file_format_t::gz:
    [[fallthrough]];
  case file_format_t::bz2:
    [[fallthrough]];
  case file_format_t::tar:
    if (!tar::TarExtract(*fd, baseOffest, afmt, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", arfile, ec);
      return 1;
    }
    return 0;
  default:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    if (!sevenzip::Decompress(arfile, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", arfile, ec);
      return 1;
    }
    break;
  }
  return 0;
}

} // namespace baulk::commands