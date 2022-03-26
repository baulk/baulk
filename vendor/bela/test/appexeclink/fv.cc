//
#include <bela/terminal.hpp>
#include <hazel/hazel.hpp>
#include <bela/datetime.hpp>
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

  bela::error_code ec;
  auto fd = bela::io::NewFile(argv[1], ec);
  if (!fd) {
    bela::FPrintF(stderr, L"unable open file %s\n", ec);
    return 1;
  }
  FILE_STANDARD_INFO di;
  if (GetFileInformationByHandleEx(fd->NativeFD(), FileStandardInfo, &di, sizeof(di)) == TRUE) {
    bela::FPrintF(stderr, L"File size: %d %b\n", di.EndOfFile.QuadPart, di.Directory);
  } else {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"GetFileInformationByHandleEx %s\n", ec);
  }

  hazel::hazel_result hr;
  if (!hazel::LookupFile(*fd, hr, ec)) {
    bela::FPrintF(stderr, L"unable detect file type %s\n", ec);
    return 1;
  }
  std::wstring space;
  space.resize(hr.align_length() + 2, ' ');
  bela::FPrintF(stderr, L"space len %d hr.align_length() %d\n", space.size(), hr.align_length());
  std::wstring_view spaceview(space);
  if (auto p = bela::RealPathByHandle(fd->NativeFD(), ec); p) {
    bela::FPrintF(stderr, L"file %v %v\n", *p, bela::integral_cast(hr.type()));
  }

  const std::wstring_view desc = L"Description";
  bela::FPrintF(stderr, L"%s:%s%s\n", desc, spaceview.substr(0, spaceview.size() - desc.size() - 1), hr.description());
  // https://en.cppreference.com/w/cpp/utility/variant/visit
  for (const auto &[k, v] : hr.values()) {
    bela::FPrintF(stderr, L"%v:%s", k, spaceview.substr(0, spaceview.size() - k.size() - 1));
    std::visit(hazel::overloaded{
                   //
                   [](auto arg) {}, //
                   [](const std::wstring &sv) { bela::FPrintF(stderr, L"%s\n", sv); },
                   [](const std::string &sv) { bela::FPrintF(stderr, L"%s\n", sv); },
                   [](int16_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](int32_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](int64_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](uint16_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](uint32_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](uint64_t i) { bela::FPrintF(stderr, L"%d\n", i); },
                   [](bela::Time t) { bela::FPrintF(stderr, L"%s\n", bela::FormatTime(t)); },
                   [spaceview](const std::vector<std::wstring> &v) {
                     if (v.empty()) {
                       bela::FPrintF(stderr, L"\n");
                       return;
                     }
                     bela::FPrintF(stderr, L"%s\n", v[0]);
                     for (size_t i = 1; i < v.size(); i++) {
                       bela::FPrintF(stderr, L"%s%s\n", spaceview, v[0]);
                     }
                   },
                   [spaceview](const std::vector<std::string> &v) {
                     if (v.empty()) {
                       bela::FPrintF(stderr, L"\n");
                       return;
                     }
                     bela::FPrintF(stderr, L"%s\n", v[0]);
                     for (size_t i = 1; i < v.size(); i++) {
                       bela::FPrintF(stderr, L"%s%s\n", spaceview, v[0]);
                     }
                   },
               },
               v);
  }
  return 0;
}