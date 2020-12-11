//
#include <bela/terminal.hpp>
#include <hazel/hazel.hpp>

// FileIdExtdDirectoryInfo

namespace winsdk {
// https://github.com/microsoft/hcsshim/blob/master/internal/winapi/filesystem.go
enum FileInfoByHandleClass {
  FileDispositionInfoEx = 21,
  FileRenameInfoEx = 22,
  FileCaseSensitiveInfo = 23,
  FileNormalizedNameInfo = 24,
  FileDispositionInformationExClass = 64
};
} // namespace winsdk

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s path\n", argv[0]);
    return 1;
  }
  hazel::io::File file;
  bela::error_code ec;
  if (!file.Open(argv[1], ec)) {
    bela::FPrintF(stderr, L"unable open file %s\n", ec.message);
    return 1;
  }
  FILE_STANDARD_INFO di;
  if (GetFileInformationByHandleEx(file.FD(), FileStandardInfo, &di, sizeof(di)) == TRUE) {
    bela::FPrintF(stderr, L"File size: %d %b\n", di.EndOfFile.QuadPart, di.Directory);
  } else {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"GetFileInformationByHandleEx %s\n", ec.message);
  }

  hazel::FileAttributeTable fat;
  if (!hazel::LookupFile(file, fat, ec)) {
    bela::FPrintF(stderr, L"unable detect file type %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"file %v %v\n", file.FullPath(), fat.type);
  for (const auto &[k, v] : fat.attributes) {
    bela::FPrintF(stderr, L"%v: %v\n", k, v);
  }
  return 0;
}