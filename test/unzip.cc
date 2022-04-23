//
#include <baulk/archive.hpp>
#include <baulk/archive/zip.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/datetime.hpp>
#include <bela/env.hpp>
#include <filesystem>

using baulk::archive::zip::File;
using baulk::archive::zip::Reader;
using bela::os::FileMode;

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
  int64_t decompressed{0};
  size_t destsize{0};
  bool owfile{false};
  bool extractFile(const File &file, bela::error_code &ec);
  bool extractDir(const File &file, std::wstring_view dir, bela::error_code &ec);
  bool extractSymlink(const File &file, std::wstring_view filename, bela::error_code &ec);
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
    auto p = bela::FullPath(path);
    if (bela::EndsWithIgnoreCase(p, L".zip")) {
      p.resize(p.size() - 4);
    } else {
      p.append(L".out");
    }
  }
  return reader.OpenReader(path, ec);
}

bool Extractor::Extract(bela::error_code &ec) {
  destsize = destination.size() + 1;
  for (const auto &file : reader.Files()) {
    if (!extractFile(file, ec)) {
      return false;
    }
  }
  return true;
}

bool Extractor::extractSymlink(const File &file, std::wstring_view filename, bela::error_code &ec) {
  std::string linkname;
  auto ret = reader.Decompress(
      file,
      [&](const void *data, size_t len) {
        linkname.append(reinterpret_cast<const char *>(data), len);
        return true;
      },
      ec);
  if (!ret) {
    return false;
  }
  return baulk::archive::NewSymlink(filename, bela::encode_into<char, wchar_t>(linkname), owfile, ec);
}

bool Extractor::extractDir(const File &file, std::wstring_view dir, bela::error_code &ec) {
  if (bela::PathExists(dir, bela::FileAttribute::Dir)) {
    return true;
  }
  std::error_code e;
  if (std::filesystem::create_directories(dir, e); e) {
    ec = bela::make_error_code_from_std(e, L"mkdir ");
    return false;
  }
  return true;
}

bool Extractor::extractFile(const File &file, bela::error_code &ec) {
  auto dest = baulk::archive::JoinSanitizePath(destination, file.name, file.IsFileNameUTF8());
  if (!dest) {
    bela::FPrintF(stderr, L"skip dangerous path %s\n", file.name);
    return true;
  }
  auto showName = std::wstring_view(dest->data() + destsize, dest->size() - destsize);
  bela::FPrintF(stderr, L" x %s\n", file.name);
  // bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", showName);
  if (file.IsSymlink()) {
    return extractSymlink(file, *dest, ec);
  }
  if (file.IsDir()) {
    return extractDir(file, *dest, ec);
  }
  auto fd = baulk::archive::File::NewFile(*dest, file.time, owfile, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"unable NewFD %s error: %s\n", *dest, ec);
    return false;
  }
  bela::error_code writeEc;
  auto ret = reader.Decompress(
      file,
      [&](const void *data, size_t len) {
        //
        return fd->WriteFull(data, len, writeEc);
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"unable Decompress %s error: %s (%s)\n", *dest, ec, writeEc);
    return false;
  }
  return true;
}

int unzip(std::wstring_view path) {
  Extractor extractor;
  if (bela::EndsWithIgnoreCase(path, L".zip")) {
    extractor.Destination() = path.substr(0, path.size() - 4);
  } else {
    extractor.Destination() = bela::StringCat(path, L".out");
  }

  extractor.OverwriteFile(true);
  bela::error_code ec;
  if (!extractor.OpenReader(path, ec)) {
    bela::FPrintF(stderr, L"unable open zip file %s error: %s\n", path, ec);
    return 1;
  }
  if (!extractor.Extract(ec)) {
    bela::FPrintF(stderr, L"unable extract file: %s error: %s\n", path, ec);
    return 1;
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