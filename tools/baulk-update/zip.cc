//
#include "baulk-update.hpp"
#include <baulk/indicators.hpp>
#include <zip.hpp>
#include <filesystem>

namespace baulk::update::zip {
using baulk::archive::zip::File;
using baulk::archive::zip::Reader;
using bela::os::FileMode;

class Extractor {
public:
  Extractor() noexcept {
    if (bela::terminal::IsSameTerminal(stderr)) {
      if (auto cygwinterminal = bela::terminal::IsCygwinTerminal(stderr); cygwinterminal) {
        CygwinTerminalSize(termsz);
      } else {
        bela::terminal::TerminalSize(stderr, termsz);
      }
    }
  }
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  std::wstring &Destination() { return destination; }
  bool OpenReader(std::wstring_view file, std::wstring_view dest, bela::error_code &ec) {
    auto zipfile = bela::PathAbsolute(file);
    destination = bela::PathAbsolute(dest);
    return reader.OpenReader(zipfile, ec);
  }
  bool Extract(bela::error_code &ec) {
    destsize = destination.size() + 1;
    for (const auto &file : reader.Files()) {
      if (!extractFile(file, ec)) {
        return false;
      }
    }
    bela::FPrintF(stderr, L"\n");
    return true;
  }

private:
  Reader reader;
  std::wstring destination;
  bela::terminal::terminal_size termsz{0};
  size_t destsize{0};
  bool owfile{true};
  bool extractFile(const File &file, bela::error_code &ec);
  bool extractDir(const File &file, std::wstring_view dir, bela::error_code &ec);
  bool extractSymlink(const File &file, std::wstring_view filename, bela::error_code &ec);
  void showProgress(std::wstring_view filename) {
    auto suglen = static_cast<size_t>(termsz.columns) - 8;
    if (auto n = bela::string_width<wchar_t>(filename); n <= suglen) {
      bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", filename);
      return;
    }
    auto basename = bela::BaseName(filename);
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...\\%s\x1b[0m", basename);
  }
};

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
  auto wn = bela::ToWide(linkname);
  if (!baulk::archive::NewSymlink(filename, wn, ec, owfile)) {
    ec = bela::make_error_code(ec.code, L"create symlink '", filename, L"' to linkname '", wn, L"' error ", ec.message);
    return false;
  }
  return true;
}

bool Extractor::extractDir(const File &file, std::wstring_view dir, bela::error_code &ec) {
  if (bela::PathExists(dir, bela::FileAttribute::Dir)) {
    return true;
  }
  std::error_code e;
  if (!std::filesystem::create_directories(dir, e)) {
    ec = bela::from_std_error_code(e, L"mkdir ");
    return false;
  }
  baulk::archive::SetFileTimeEx(dir, file.time, ec);
  return true;
}

bool Extractor::extractFile(const File &file, bela::error_code &ec) {
  auto dest = baulk::archive::zip::JoinSanitizePath(destination, file);
  if (!dest) {
    bela::FPrintF(stderr, L"skip dangerous path %s\n", file.name);
    return true;
  }
  auto showName = std::wstring_view(dest->data() + destsize, dest->size() - destsize);
  if (termsz.columns != 0) {
    showProgress(showName);
  }
  if (file.IsSymlink()) {
    return extractSymlink(file, *dest, ec);
  }
  if (file.IsDir()) {
    return extractDir(file, *dest, ec);
  }
  auto fd = baulk::archive::NewFD(*dest, ec, owfile);
  if (!fd) {
    bela::FPrintF(stderr, L"unable NewFD %s error: %s\n", *dest, ec.message);
    return false;
  }
  if (!fd->SetTime(file.time, ec)) {
    bela::FPrintF(stderr, L"unable SetTime %s error: %s\n", *dest, ec.message);
    return false;
  }
  bela::error_code ec2;
  auto ret = reader.Decompress(
      file,
      [&](const void *data, size_t len) {
        //
        return fd->Write(data, len, ec2);
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"unable decompress %s error: %s (%s)\n", *dest, ec.message, ec2.message);
    return false;
  }
  return true;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  Extractor extractor;
  if (!extractor.OpenReader(src, outdir, ec)) {
    return false;
  }
  return extractor.Extract(ec);
}
} // namespace baulk::update::zip