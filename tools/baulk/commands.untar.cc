//
#include "baulk.hpp"
#include "commands.hpp"
#include "tar.hpp"

namespace baulk::commands {
inline std::wstring resolveDestination(const argv_t &argv, std::wstring_view tarfile) {
  if (argv.size() > 1) {
    return bela::PathAbsolute(argv[1]);
  }
  if (auto d = baulk::archive::tar::PathRemoveExtension(tarfile); d.size() != tarfile.size()) {
    return std::wstring(d);
  }
  return bela::StringCat(tarfile, L".out");
}

int cmd_untar(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk untar tarfile dest\n");
    return 1;
  }
  auto file = bela::PathAbsolute(argv[0]);
  auto root = resolveDestination(argv, file);
  DbgPrint(L"destination %s", root);
  bela::error_code ec;
  auto fr = baulk::archive::tar::OpenFile(file, ec);
  if (fr == nullptr) {
    bela::FPrintF(stderr, L"unable open file %s error %s\n", file, ec.message);
    return 1;
  }
  auto wr = baulk::archive::tar::MakeReader(*fr, ec);
  std::shared_ptr<baulk::archive::tar::Reader> tr;
  if (wr != nullptr) {
    tr = std::make_shared<baulk::archive::tar::Reader>(wr.get());
  } else if (ec.code == baulk::archive::tar::ErrNoFilter) {
    tr = std::make_shared<baulk::archive::tar::Reader>(fr.get());
  } else {
    bela::FPrintF(stderr, L"unable open tar file %s error %s\n", file, ec.message);
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
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", fh->Name);
    auto dest = baulk::archive::tar::PathRemoveExtension(file);
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
  return 0;
}
} // namespace baulk::commands