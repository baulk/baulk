/// ----- support tar.*
#include <string_view>
#include <bela/path.hpp>
#include <regutils.hpp>
#include <tar.hpp>
#include "baulk.hpp"
#include "decompress.hpp"
#include "fs.hpp"
#include "indicators.hpp"

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
  if (auto n = bela::StringWidth(filename); n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", filename);
    return;
  }
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...\\%s\x1b[0m", BaseName(filename));
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
    bela::FPrintF(stderr, L"unable open file %s error %s\n", src, ec.message);
    return 1;
  }
  auto wr = baulk::archive::tar::MakeReader(*fr, ec);
  std::shared_ptr<baulk::archive::tar::Reader> tr;
  if (wr != nullptr) {
    tr = std::make_shared<baulk::archive::tar::Reader>(wr.get());
  } else if (ec.code == baulk::archive::tar::ErrNoFilter) {
    tr = std::make_shared<baulk::archive::tar::Reader>(fr.get());
  } else {
    bela::FPrintF(stderr, L"unable open tar file %s error %s\n", src, ec.message);
    return 1;
  }
  for (;;) {
    auto fh = tr->Next(ec);
    if (!fh) {
      if (ec.code == bela::ErrEnded) {
        break;
      }
      bela::FPrintF(stderr, L"\nuntar error %s\n", ec.message);
      break;
    }
    showProgress(termsz, fh->Name);
    auto out = baulk::archive::PathCat(dest, fh->Name);
    if (!out) {
      continue;
    }
    if (fh->Size == 0) {
      continue;
    }
    auto fd = baulk::archive::NewFD(*out, ec, true);
    if (!fd) {
      bela::FPrintF(stderr, L"newFD %s error: %s\n", *out, ec.message);
      continue;
    }
    fd->SetTime(fh->ModTime, ec);
    if (!tr->WriteTo(
            [&](const void *data, size_t len, bela::error_code &ec) -> bool {
              //
              return fd->Write(data, len, ec);
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
