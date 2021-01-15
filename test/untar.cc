///
#include <tar.hpp>
#include <bela/terminal.hpp>
#include <bela/datetime.hpp>

bool untar(std::wstring_view file) {
  bela::FPrintF(stderr, L"untar %s\n", file);
  bela::error_code ec;
  auto fr = baulk::archive::tar::OpenFile(file, ec);
  if (fr == nullptr) {
    bela::FPrintF(stderr, L"unable open file %s error %s\n", file, ec.message);
    return false;
  }
  bela::FPrintF(stderr, L"File size: %d\n", fr->Size());
  auto wr = baulk::archive::tar::MakeReader(*fr, ec);
  std::shared_ptr<baulk::archive::tar::Reader> tr;
  if (wr != nullptr) {
    tr = std::make_shared<baulk::archive::tar::Reader>(wr.get());
  } else {
    tr = std::make_shared<baulk::archive::tar::Reader>(fr.get());
  }
  for (;;) {
    auto fh = tr->Next(ec);
    if (!fh) {
      bela::FPrintF(stderr, L"untar %s\n", ec.message);
      break;
    }
    bela::FPrintF(stderr, L"Filename: %s %s %d\n", fh->Name, bela::FormatTime(fh->ModeTime), fh->Size);
    if (fh->Size != 0) {
      auto size = fh->Size;
      char buffer[4096];
      while (size > 0) {
        auto msz = (std::min)(size, 4096ll);
        auto n = tr->Read(buffer, msz, ec);
        if (n <= 0) {
          break;
        }
        size -= n;
      }
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