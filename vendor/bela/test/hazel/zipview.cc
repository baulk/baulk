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
  hazel::io::File file;
  bela::error_code ec;
  if (!file.Open(argv[1], ec)) {
    bela::FPrintF(stderr, L"unable openfile: %s %s\n", argv[1], ec.message);
    return 1;
  }
  std::wstring path;
  if (auto fp = bela::RealPathByHandle(file.FD(), ec)) {
    path.assign(std::move(*fp));
  }
  hazel::FileAttributeTable fat;
  if (!hazel::LookupFile(file, fat, ec)) {
    bela::FPrintF(stderr, L"unable detect file type: %s %s\n", argv[1], ec.message);
    return 1;
  }
  if (!fat.LooksLikeZIP()) {
    bela::FPrintF(stderr, L"file: %s not zip file\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stderr, L"sizeof(zip::Reader) = %d %d %d\n", sizeof(hazel::zip::Reader), sizeof(std::string),
                sizeof(std::vector<hazel::zip::File>));
  auto zr = hazel::zip::NewReader(file.FD(), fat.size, ec);
  if (!zr) {
    bela::FPrintF(stderr, L"open zip file: %s error %s\n", path, ec.message);
    return 1;
  }
  if (!zr->Comment().empty()) {
    bela::FPrintF(stderr, L"comment: %s\n", zr->Comment());
  }

  for (const auto &file : zr->Files()) {
    if (file.IsEncrypted()) {
      bela::FPrintF(stderr, L"File: %s [%s] (%s %s) %d\n", file.name, bela::FormatTime(file.time),
                    hazel::zip::Method(file.method), file.AesText(), file.uncompressedSize);
      continue;
    }
    bela::FPrintF(stderr, L"File: %s [%s|%s] (%s) %d\n", file.name, bela::FormatTime(file.time),
                  bela::FormatUniversalTime(file.time), hazel::zip::Method(file.method), file.uncompressedSize);
  }
  switch (zr->LooksLikeOffice()) {
  case hazel::zip::OfficeDocx:
    bela::FPrintF(stderr, L"File is Microsoft Office Word (2007+)\n");
    break;
  case hazel::zip::OfficePptx:
    bela::FPrintF(stderr, L"File is Microsoft Office PowerPoint (2007+)\n");
    break;
  case hazel::zip::OfficeXlsx:
    bela::FPrintF(stderr, L"File is Microsoft Office Excel (2007+)\n");
    break;
  default:
    break;
  }
  if (zr->LooksLikeOFD()) {
    bela::FPrintF(stderr, L"File is Open Fixed-layout Document (GB/T 33190-2016)\n");
  }
  if (zr->LooksLikeAppx()) {
    bela::FPrintF(stderr, L"File is Windows App Packages\n");
  }
  if (zr->LooksLikeApk()) {
    bela::FPrintF(stderr, L"File is Android APK\n");
  } else if (zr->LooksLikeJar()) {
    bela::FPrintF(stderr, L"File is Java Jar\n");
  }
  bela::FPrintF(stderr, L"Files: %d\n", zr->Files().size());
  return 0;
}