///
#include <baulk/archive.hpp>
#include <baulk/archive/tar.hpp>
#include <bela/terminal.hpp>
#include <bela/datetime.hpp>

bool untar(std::wstring_view file) {
  bela::FPrintF(stderr, L"untar %s\n", file);
  bela::error_code ec;
  int64_t offset = 0;
  baulk::archive::file_format_t afmt{baulk::archive::file_format_t::none};
  auto fd = baulk::archive::OpenFile(file, offset, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"unable open file %s error %s\n", file, ec);
    return false;
  }
  if (afmt == baulk::archive::file_format_t::none) {
    if (bela::EndsWithIgnoreCase(file, L".tar.br") || bela::EndsWithIgnoreCase(file, L".tbr")) {
      afmt = baulk::archive::file_format_t::brotli;
    }
  }
  baulk::archive::tar::FileReader fr(fd->NativeFD());
  auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec);
  std::shared_ptr<baulk::archive::tar::Reader> tr;
  if (wr != nullptr) {
    tr = std::make_shared<baulk::archive::tar::Reader>(wr.get());
  } else if (ec.code == baulk::archive::tar::ErrNoFilter) {
    tr = std::make_shared<baulk::archive::tar::Reader>(&fr);
  } else {
    bela::FPrintF(stderr, L"unable open tar file %s error %s\n", file, ec);
    return false;
  }
  for (;;) {
    auto fh = tr->Next(ec);
    if (!fh) {
      if (ec.code == bela::ErrEnded) {
        bela::FPrintF(stderr, L"\nsuccess\n");
        break;
      }
      bela::FPrintF(stderr, L"\nuntar error %s\n", ec);
      break;
    }
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", fh->Name);
    auto dest = baulk::archive::PathStripExtension(file);
    auto out = baulk::archive::JoinSanitizePath(dest, fh->Name);
    if (!out) {
      continue;
    }
    if (fh->Size == 0) {
      continue;
    }
    auto fd = baulk::archive::File::NewFile(*out, fh->ModTime, true, ec);
    if (!fd) {
      bela::FPrintF(stderr, L"newFD %s error: %s\n", *out, ec);
      continue;
    }
    // auto size = fh->Size;
    // char buffer[4096];
    // while (size > 0) {
    //   auto minsize = (std::min)(size, 4096ll);
    //   auto n = tr->Read(buffer, minsize, ec);
    //   if (n <= 0) {
    //     break;
    //   }
    //   size -= n;
    //   if (!fd->Write(buffer, n, ec)) {
    //     fd->Discard();
    //   }
    // }
    if (!tr->WriteTo(
            [&](const void *data, size_t len, bela::error_code &ec) -> bool {
              //
              return fd->WriteFull(data, len, ec);
            },
            fh->Size, ec)) {
      fd->Discard();
    }
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s tarfile\n", argv[0]);
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    if (!untar(argv[i])) {
      return 1;
    }
  }
  return 0;
}