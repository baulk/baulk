//
#include <hazel/hazel.hpp>
#include <hazel/zip.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/datetime.hpp>

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

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s zipfile\n", argv[0]);
    return 1;
  }
  bela::File file;
  bela::error_code ec;
  if (!file.Open(argv[1], ec)) {
    bela::FPrintF(stderr, L"unable openfile: %s %s\n", argv[1], ec.message);
    return 1;
  }
  std::wstring path;
  if (auto fp = bela::RealPathByHandle(file.FD(), ec)) {
    path.assign(std::move(*fp));
  }
  hazel::hazel_result hr;
  if (!hazel::LookupFile(file, hr, ec)) {
    bela::FPrintF(stderr, L"unable detect file type: %s %s\n", argv[1], ec.message);
    return 1;
  }
  if (!hr.LooksLikeZIP()) {
    bela::FPrintF(stderr, L"file: %s not zip file\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stderr, L"sizeof(zip::Reader) = %d %d %d\n", sizeof(hazel::zip::Reader), sizeof(std::string),
                sizeof(std::vector<hazel::zip::File>));
  hazel::zip::Reader zr;
  if (!zr.OpenReader(file.FD(), hr.size(), ec)) {
    bela::FPrintF(stderr, L"open zip file: %s error %s\n", path, ec.message);
    return 1;
  }
  if (!zr.Comment().empty()) {
    bela::FPrintF(stdout, L"comment: %s\n", zr.Comment());
  }

  for (const auto &file : zr.Files()) {
    if (file.IsEncrypted()) {
      bela::FPrintF(stdout, L"%s\t%s\t%d\t%s\t%s\t%b\t%s\n", hazel::zip::String(file.mode),
                    hazel::zip::Method(file.method), file.uncompressedSize, bela::FormatTime(file.time), file.name,
                    file.IsFileNameUTF8(), file.AesText());
      continue;
    }
    bela::FPrintF(stdout, L"%s\t%s\t%d\t%s\t%s\t%b\n", hazel::zip::String(file.mode), hazel::zip::Method(file.method),
                  file.uncompressedSize, bela::FormatTime(file.time), file.name, file.IsFileNameUTF8());
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