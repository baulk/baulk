//
#include <hazel/hazel.hpp>
#include <hazel/zip.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/datetime.hpp>
#include <bela/pe.hpp>

inline std::string TimeString(time_t t) {
  if (t < 0) {
    t = 0;
  }
  std::tm tm_;
  localtime_s(&tm_, &t);

  std::string buffer;
  buffer.resize(64);
  auto n = std::strftime(buffer.data(), 64, "%Y-%m-%dT%H:%M:%S%z", &tm_);
  buffer.resize(n);
  return buffer;
}

int listArchive(std::wstring_view path, HANDLE fd, int64_t size, int64_t offset) {
  bela::error_code ec;
  hazel::zip::Reader zr;
  if (!zr.OpenReader(fd, size, offset, ec)) {
    bela::FPrintF(stderr, L"open zip file: %s error %s\n", path, ec);
    return 1;
  }
  if (!zr.Comment().empty()) {
    bela::FPrintF(stdout, L"comment: %s\n", zr.Comment());
  }

  for (const auto &file : zr.Files()) {
    if (file.IsEncrypted()) {
      bela::FPrintF(stdout, L"%s\t%s\t%d\t%s\t%s\t%b\t%s\n", hazel::zip::String(file.mode),
                    hazel::zip::Method(file.method), file.uncompressed_size, bela::FormatTime(file.time), file.name,
                    file.IsFileNameUTF8(), file.AesText());
      continue;
    }
    bela::FPrintF(stdout, L"%s\t%s\t%d\t%s\t%s\t%b\n", hazel::zip::String(file.mode), hazel::zip::Method(file.method),
                  file.uncompressed_size, bela::FormatTime(file.time), file.name, file.IsFileNameUTF8());
  }
  switch (zr.LooksLikeMsZipContainer()) {
  case hazel::zip::OfficeDocx:
    bela::FPrintF(stdout, L"File is Microsoft Office Word (2007+)\n");
    break;
  case hazel::zip::OfficePptx:
    bela::FPrintF(stdout, L"File is Microsoft Office PowerPoint (2007+)\n");
    break;
  case hazel::zip::OfficeXlsx:
    bela::FPrintF(stdout, L"File is Microsoft Office Excel (2007+)\n");
    break;
  case hazel::zip::NuGetPackage:
    bela::FPrintF(stdout, L"File is NuGet Package\n");
    break;
  default:
    break;
  }
  if (zr.LooksLikeOFD()) {
    bela::FPrintF(stdout, L"File is Open Fixed-layout Document (GB/T 33190-2016)\n");
  }
  if (zr.LooksLikeAppx()) {
    bela::FPrintF(stdout, L"File is Windows App Packages\n");
  }
  if (zr.LooksLikeApk()) {
    bela::FPrintF(stdout, L"File is Android APK\n");
  } else if (zr.LooksLikeJar()) {
    bela::FPrintF(stdout, L"File is Java Jar\n");
  }
  std::string odfmime;
  if (zr.LooksLikeODF(&odfmime)) {
    bela::FPrintF(stdout, L"File is OpenDocument Format, mime: %s\n", odfmime);
  }
  bela::FPrintF(stdout, L"Files: %d CompressedSize: %d UncompressedSize: %d\n", zr.Files().size(), zr.CompressedSize(),
                zr.UncompressedSize());
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s zipfile\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto fd = bela::io::NewFile(argv[1], ec);
  if (!fd) {
    bela::FPrintF(stderr, L"unable open file %s\n", ec);
    return 1;
  }
  char magic[10] = {0};
  int64_t outlen = 0;
  if (fd->ReadAt(magic, 0, outlen, ec)) {
    bela::FPrintF(stderr, L"magic: '%s' outlen: %d\n", std::string_view{magic, 4}, outlen);
  }
  std::wstring path;
  if (auto p = bela::RealPathByHandle(fd->NativeFD(), ec)) {
    path.assign(std::move(*p));
  }
  hazel::hazel_result hr;
  if (!hazel::LookupFile(*fd, hr, ec)) {
    bela::FPrintF(stderr, L"unable detect file type: %s %s\n", argv[1], ec);
    return 1;
  }
  if (hr.LooksLikeZIP()) {

    return listArchive(path, fd->NativeFD(), hr.size(), 0);
  }
  if (!hr.LooksLikePE()) {
    bela::FPrintF(stderr, L"file: %s not zip file\n", argv[1]);
    return 1;
  }
  bela::pe::File file;
  if (!file.NewFile(fd->NativeFD(), hr.size(), ec)) {
    bela::FPrintF(stderr, L"open pe file: %s error: %s\n", argv[1], ec);
    return 1;
  }
  if (file.OverlayLength() < 4) {
    bela::FPrintF(stderr, L"file: %s not zip file\n", argv[1]);
    return 1;
  }
  return listArchive(path, fd->NativeFD(), hr.size(), file.OverlayOffset());
}