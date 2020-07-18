///
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/codecvt.hpp>
#include <bela/path.hpp>
#include <zip.hpp>
#include "indicators.hpp"
#include "fs.hpp"
#include "baulk.hpp"

namespace baulk::zip {
// avoid filename too long
int32_t OnEntry(void *handle, void *userdata, mz_zip_file *file_info, const char *path) {
  auto termsz = reinterpret_cast<const bela::terminal::terminal_size *>(userdata);
  if (termsz == nullptr || termsz->columns <= 8) {
    return 0;
  }
  auto suglen = static_cast<size_t>(termsz->columns) - 8;
  auto filename = bela::ToWide(std::string_view{file_info->filename, file_info->filename_size});
  if (auto n = bela::StringWidth(filename); n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", filename);
    return 0;
  }
  auto basename = bela::BaseName(filename);
  auto n = bela::StringWidth(basename);
  if (n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...\\%s\x1b[0m", basename);
    return 0;
  }
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...%s\x1b[0m", basename.substr(n - suglen));
  return 0;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  bela::terminal::terminal_size termsz{0};
  baulk::archive::zip::zip_closure closure{nullptr};
  if (!baulk::IsQuietMode) {
    if (bela::terminal::IsSameTerminal(stderr)) {
      if (auto cygwinterminal = bela::terminal::IsCygwinTerminal(stderr); cygwinterminal) {
        CygwinTerminalSize(termsz);
      } else {
        bela::terminal::TerminalSize(stderr, termsz);
      }
    }
    closure.userdata = &termsz;
    closure.entry = OnEntry;
  }
  if (!baulk::archive::zip::ZipExtract(src, outdir, ec, &closure)) {
    return false;
  }
  if (!baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}

} // namespace baulk::zip