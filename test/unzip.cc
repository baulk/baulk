//
#include <zip.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>

struct context {
  int64_t total{0};
};

int32_t on_entry(void *handle, void *userdata, mz_zip_file *file_info,
                 const char *path) {
  bela::FPrintF(stderr, L"\x1b[2K\rx %s", file_info->filename);
  return 0;
}

int32_t on_progress(void *handle, void *userdata, mz_zip_file *file_info,
                    int64_t position) {
  uint8_t raw = 0;
  mz_zip_reader_get_raw(handle, &raw);
  double progress = 0;
  if (!raw && file_info->uncompressed_size > 0) {
    progress = ((double)position / file_info->uncompressed_size) * 100;
    /* Print the progress of the current extraction */
    bela::FPrintF(stderr, L"\x1b[2K\r%s - %d/%d (%.02f%%)", file_info->filename,
                  position, file_info->uncompressed_size, progress);
  }

  return 0;
}

int do_unzip(std::wstring_view file) {
  auto FileHandle =
      CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"unable open file %s\n", ec.message);
    return 1;
  }
  bela::error_code ec;
  baulk::archive::zip::zip_closure closure{nullptr, on_progress, nullptr};
  auto p = bela::PathAbsolute(file);
  p.append(L".old");
  if (!baulk::archive::zip::ZipExtract(file, p, ec, &closure)) {
    bela::FPrintF(stderr, L"zipExtract %s\n", ec.message);
    return 1;
  }
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s zipfile\n", argv[0]);
    return 1;
  }
  return do_unzip(argv[1]);
}