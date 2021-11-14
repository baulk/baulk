/// ----- support tar.*
#include <bela/path.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/tar.hpp>
#include <baulk/indicators.hpp>
#include "baulk.hpp"
#include "decompress.hpp"

namespace baulk::tar {

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

bool extractSymlink(std::wstring_view filename, std::string_view linkname, bela::error_code &ec) {
  auto wn = bela::encode_into<char, wchar_t>(linkname);
  if (!baulk::archive::NewSymlink(filename, wn, ec, true)) {
    return false;
  }
  return true;
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

bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  if (!baulk::fs::MakeDir(dest, ec)) {
    return false;
  }
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
  DbgPrint(L"destination %s", dest);
  auto fr = baulk::archive::tar::OpenFile(src, ec);
  if (fr == nullptr) {
    bela::FPrintF(stderr, L"unable open file %s error %s\n", src, ec);
    return 1;
  }
  auto wr = baulk::archive::tar::MakeReader(*fr, 0, ec);
  std::shared_ptr<baulk::archive::tar::Reader> tr;
  if (wr != nullptr) {
    tr = std::make_shared<baulk::archive::tar::Reader>(wr.get());
  } else if (ec.code == baulk::archive::tar::ErrNoFilter) {
    tr = std::make_shared<baulk::archive::tar::Reader>(fr.get());
  } else {
    bela::FPrintF(stderr, L"unable open tar file %s error %s\n", src, ec);
    return 1;
  }
  for (;;) {
    auto fh = tr->Next(ec);
    if (!fh) {
      if (ec.code == bela::ErrEnded) {
        break;
      }
      bela::FPrintF(stderr, L"\nuntar error %s\n", ec);
      break;
    }
    showProgress(termsz, fh->Name);
    auto out = baulk::archive::JoinSanitizePath(dest, fh->Name);
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
      bela::FPrintF(stderr, L"newFD %s error: %s\n", *out, ec);
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
  if (!baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}
} // namespace baulk::tar
