/// ----- support tar.*
#include <bela/path.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/tar.hpp>
#include <baulk/indicators.hpp>
#include "baulk.hpp"
#include "extract.hpp"

namespace baulk {

std::string_view BaseName(std::string_view name) {
  if (name.empty()) {
    return ".";
  }
  if (name.size() == 2 && name[1] == ':') {
    return ".";
  }
  if (name.size() > 2 && name[1] == ':') {
    name.remove_prefix(2);
  }
  auto i = name.size() - 1;
  for (; i != 0 && bela::IsPathSeparator(name[i]); i--) {
    name.remove_suffix(1);
  }
  if (name.size() == 1 && bela::IsPathSeparator(name[0])) {
    return ".";
  }
  for (; i != 0 && !bela::IsPathSeparator(name[i - 1]); i--) {
  }
  return name.substr(i);
}

void showProgress(const bela::terminal::terminal_size &termsz, std::string_view filename) {
  auto suglen = static_cast<size_t>(termsz.columns) - 8;
  if (auto n = bela::string_width<char>(filename); n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", filename);
    return;
  }
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...\\%s\x1b[0m", BaseName(filename));
}

inline bool extractSymlink(std::wstring_view filename, std::string_view linkname, bela::error_code &ec) {
  return baulk::archive::NewSymlink(filename, bela::encode_into<char, wchar_t>(linkname), ec, true);
}

bool extractDir(std::wstring_view dir, bela::Time t, bela::error_code &ec) {
  if (bela::PathExists(dir, bela::FileAttribute::Dir)) {
    return true;
  }
  std::error_code e;
  if (!std::filesystem::create_directories(dir, e)) {
    ec = bela::from_std_error_code(e, L"mkdir ");
    return false;
  }
  baulk::archive::Chtimes(dir, t, ec);
  return true;
}

constexpr auto ErrParseCompressedFile = baulk::archive::tar::ErrNoFilter + 10000;

bool extract_tar(baulk::archive::tar::ExtractReader *reader, std::wstring_view dest, bela::error_code &ec) {
  bela::terminal::terminal_size termsz{0};
  if (!baulk::IsQuietMode) {
    if (bela::terminal::IsSameTerminal(stderr)) {
      if (auto cygwinterminal = bela::terminal::IsCygwinTerminal(stderr); cygwinterminal) {
        CygwinTerminalSize(termsz);
      } else {
        bela::terminal::TerminalSize(stderr, termsz);
      }
    }
  }
  auto destination = bela::PathAbsolute(dest);
  auto tr = std::make_shared<baulk::archive::tar::Reader>(reader);
  for (;;) {
    auto fh = tr->Next(ec);
    if (!fh) {
      if (ec.code == bela::ErrEnded) {
        break;
      }
      break;
    }
    showProgress(termsz, fh->Name);
    auto out = baulk::archive::JoinSanitizePath(destination, fh->Name);
    if (!out) {
      continue;
    }
    if (fh->IsDir()) {
      bela::error_code le;
      if (!extractDir(*out, fh->ModTime, le)) {
        bela::FPrintF(stderr, L"\nuntar mkdir %s\n", ec);
      }
      continue;
    }
    if (fh->IsSymlink()) {
      bela::error_code le;
      if (!extractSymlink(*out, fh->LinkName, ec)) {
        bela::FPrintF(stderr, L"\nuntar mklink %s\n", ec);
      }
      continue;
    }
    // i
    if (!fh->IsRegular()) {
      continue;
    }
    auto fd = baulk::archive::File::NewFile(*out, true, ec);
    if (!fd) {
      bela::FPrintF(stderr, L"NewFD %s error: %s\n", *out, ec);
      continue;
    }
    fd->Chtimes(fh->ModTime, ec);
    if (!tr->WriteTo(
            [&](const void *data, size_t len, bela::error_code &ec) -> bool {
              //
              return fd->WriteFull(data, len, ec);
            },
            fh->Size, ec)) {
      fd->Discard();
    }
  }
  if (tr->Index() == 0) {
    ec = bela::make_error_code(ErrParseCompressedFile, L"single decompress file");
    return false;
  }
  if (!ec) {
    bela::FPrintF(stderr, L"\nuntar error %s\n", ec);
    return false;
  }
  if (!baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}

bool single_decompress(std::wstring_view src, baulk::archive::tar::FileReader &fr, int64_t offset,
                       baulk::archive::file_format_t afmt, std::wstring_view dest, bela::error_code &ec) {
  auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec);
  if (!wr) {
    bela::FPrintF(stderr, L"unable decompress file: %s no filter\n", src);
    return false;
  }
  auto baseName = baulk::archive::PathRemoveExtension(bela::BaseName(src));
  DbgPrint(L"File %s not tar file", bela::BaseName(src));
  bela::FPrintF(stderr, L"\x1b[33mx %s\x1b[0m\n", baseName);
  auto out = bela::StringCat(dest, L"\\", baseName);
  auto fd = baulk::archive::File::NewFile(out, true, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"NewFD %s error: %s\n", out, ec);
    return false;
  }
  uint8_t buffer[8192];
  for (;;) {
    auto nBytes = wr->Read(buffer, sizeof(buffer), ec);
    if (nBytes <= 0) {
      break;
    }
    if (!fd->WriteFull(buffer, static_cast<size_t>(nBytes), ec)) {
      return false;
    }
  }
  return true;
}

bool extract_tar(const bela::io::FD &fd, int64_t offset, baulk::archive::file_format_t afmt, std::wstring_view src,
                 std::wstring_view dest, bela::error_code &ec) {
  DbgPrint(L"destination %s", dest);
  if (!baulk::fs::MakeDir(dest, ec)) {
    return false;
  }
  baulk::archive::tar::FileReader fr(fd.NativeFD());
  if (auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec); wr) {
    if (extract_tar(wr.get(), dest, ec)) {
      return true;
    }
    if (ec.code != ErrParseCompressedFile) {
      return false;
    }
    return single_decompress(src, fr, offset, afmt, dest, ec);
  }
  if (ec.code == baulk::archive::tar::ErrNoFilter) {
    return extract_tar(&fr, dest, ec);
  }
  bela::FPrintF(stderr, L"untar error %s\n", ec);
  return false;
}

bool extract_tar(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  int64_t offset = 0;
  baulk::archive::file_format_t afmt{baulk::archive::file_format_t::none};
  if (auto fd = baulk::archive::OpenArchiveFile(src, offset, afmt, ec); fd) {
    if (afmt == baulk::archive::file_format_t::none) {
      if (bela::EndsWithIgnoreCase(src, L".tar.br") || bela::EndsWithIgnoreCase(src, L".tbr")) {
        afmt = baulk::archive::file_format_t::brotli;
      }
    }
    return extract_tar(*fd, offset, afmt, src, dest, ec);
  }
  bela::FPrintF(stderr, L"unable open tar file %s error %s\n", src, ec);
  return false;
}
} // namespace baulk
