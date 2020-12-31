//
#include <zip.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/datetime.hpp>

using baulk::archive::FileMode;
using baulk::archive::zip::File;
using baulk::archive::zip::Reader;

class Extractor {
public:
  Extractor() noexcept = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  std::wstring &Destination() { return destination; }
  bool OpenReader(std::wstring_view path, bela::error_code &ec);
  bool Extract(bela::error_code &ec);
  bool OverwriteFile(bool ow) {
    owfile = ow;
    return owfile;
  }

private:
  Reader reader;
  std::wstring destination;
  bool owfile{false};
  bool extractFile(const File &file, bela::error_code &ec);
  bool extractDir(const File &file, std::wstring_view dir, bela::error_code &ec);
};

FILETIME TimeToFileTime(bela::Time t) {
  FILETIME ft{};
  auto parts = bela::Split(t);
  auto tick = (parts.sec - 11644473600ll) * 10000000 + parts.nsec / 100;
  ft.dwHighDateTime = static_cast<DWORD>(tick << 32);
  ft.dwLowDateTime = static_cast<DWORD>(tick);
  return ft;
}

bool Extractor::OpenReader(std::wstring_view path, bela::error_code &ec) {
  if (destination.empty()) {
    auto p = bela::PathAbsolute(path);
    if (bela::EndsWithIgnoreCase(p, L".zip")) {
      p.resize(p.size() - 4);
    } else {
      p.append(L".out");
    }
  }
  return reader.OpenReader(path, ec);
}

bool Extractor::Extract(bela::error_code &ec) {
  for (const auto &file : reader.Files()) {
    if (!extractFile(file, ec)) {
      return false;
    }
  }
  return true;
}

bool Extractor::extractDir(const File &file, std::wstring_view dir, bela::error_code &ec) {
  //

  return true;
}

bool Extractor::extractFile(const File &file, bela::error_code &ec) {
  auto dest = baulk::archive::zip::PathCat(destination, file);
  if (!dest) {
    bela::FPrintF(stderr, L"skip dangerous path %s\n", file.name);
    return true;
  }
  if (file.IsDir()) {

    return extractDir(file, *dest, ec);
  }
  return true;
}

int unzip(std::wstring_view path) {
  std::wstring out;
  if (bela::EndsWithIgnoreCase(path, L".zip")) {
    out = path.substr(0, path.size() - 4);
  } else {
    out.assign(path).append(L".out");
  }
  baulk::archive::zip::Reader zr;
  bela::error_code ec;
  if (!zr.OpenReader(path, ec)) {
    bela::FPrintF(stderr, L"unable open zip file %s error: %s\n", path, ec.message);
    return 1;
  }
  for (const auto &file : zr.Files()) {
    auto filepath = baulk::archive::zip::PathCat(out, file);
    if (!filepath) {
      bela::FPrintF(stderr, L"dangerous path %s\n", file.name);
      continue;
    }
  }
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s zipfile\n", argv[0]);
    return 1;
  }
  return unzip(argv[1]);
}