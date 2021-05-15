///
#include "baulk.hpp"
#include "commands.hpp"
#include <zip.hpp>
#include <bela/match.hpp>
#include <filesystem>

namespace baulk::commands {

using baulk::archive::zip::File;
using baulk::archive::zip::Reader;
using bela::os::FileMode;

class Extractor {
public:
  Extractor() noexcept = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  std::wstring &Destination() { return destination; }
  bool OpenReader(const argv_t &argv, bela::error_code &ec);
  bool Extract(bela::error_code &ec);

private:
  Reader reader;
  std::wstring destination;
  int64_t decompressed{0};
  size_t destsize{0};
  bool owfile{true};
  bool extractFile(const File &file, bela::error_code &ec);
  bool extractDir(const File &file, std::wstring_view dir, bela::error_code &ec);
  bool extractSymlink(const File &file, std::wstring_view filename, bela::error_code &ec);
};

inline std::wstring resolveZipDestination(const argv_t &argv, std::wstring_view zipfile) {
  if (argv.size() > 1) {
    return bela::PathAbsolute(argv[1]);
  }
  if (bela::EndsWithIgnoreCase(zipfile, L".zip")) {
    return std::wstring(zipfile.data(), zipfile.size() - 4);
  }
  return bela::StringCat(zipfile, L".out");
}

bool Extractor::OpenReader(const argv_t &argv, bela::error_code &ec) {
  auto zipfile = bela::PathAbsolute(argv[0]);
  destination = resolveZipDestination(argv, zipfile);
  return reader.OpenReader(zipfile, ec);
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
  return true;
}

bool Extractor::extractFile(const File &file, bela::error_code &ec) {
  auto dest = baulk::archive::zip::PathCat(destination, file);
  if (!dest) {
    bela::FPrintF(stderr, L"skip dangerous path %s\n", file.name);
    return true;
  }
  auto showName = std::wstring_view(dest->data() + destsize, dest->size() - destsize);
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", showName);
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
    bela::FPrintF(stderr, L"unable Decompress %s error: %s (%s)\n", *dest, ec.message, ec2.message);
    return false;
  }
  return true;
}

void usage_unzip() {
  bela::FPrintF(stderr, LR"(Usage: baulk unzip zipfile destination
Extract compressed files in a ZIP archive (experimental).

Example:
  baulk unzip curl-7.76.0.zip
  baulk unzip curl-7.76.0.zip curl-dest

)");
}

int cmd_unzip(const argv_t &argv) {
  if (argv.empty()) {
    usage_unzip();
    return 1;
  }
  bela::error_code ec;
  Extractor extractor;
  if (!extractor.OpenReader(argv, ec)) {
    bela::FPrintF(stderr, L"unable open zip file %s error: %s\n", argv[0], ec.message);
    return 1;
  }
  if (!extractor.Extract(ec)) {
    bela::FPrintF(stderr, L"unable extract file: %s error: %s\n", argv[0], ec.message);
    return 1;
  }
  return 0;
}
} // namespace baulk::commands