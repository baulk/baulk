///
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/terminal.hpp>
#include <zip.hpp>
#include "fs.hpp"

namespace baulk::zip {
int32_t OnEntry(void *handle, void *userdata, mz_zip_file *file_info,
                const char *path) {
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", file_info->filename);
  return 0;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec) {
  baulk::archive::zip::zip_closure closure{nullptr, nullptr, OnEntry};
  auto flush = bela::finally([] { bela::FPrintF(stderr, L"\n"); });
  return baulk::archive::zip::ZipExtract(src, outdir, ec, &closure);
}

} // namespace baulk::zip