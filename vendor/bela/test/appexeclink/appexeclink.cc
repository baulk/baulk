///
#include <bela/terminal.hpp>
#include <bela/path.hpp>
// test\appexeclink\appexeclink_test.exe
// C:\Users\$Username\AppData\Local\Microsoft\WindowsApps\wt.exe
int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s appexeclink\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto p = bela::RealPath(argv[1], ec);
  if (p) {
    bela::FPrintF(stdout, L"%s RealPath [%s]\n", argv[1], *p);
  } else {
    bela::FPrintF(stderr, L"%s RealPath error: %s\n", argv[1], ec.message);
  }
  auto p2 = bela::RealPathEx(argv[1], ec);
  if (p2) {
    bela::FPrintF(stdout, L"%s RealPathEx [%s]\n", argv[1], *p2);
  } else {
    bela::FPrintF(stderr, L"%s RealPathEx error: %s\n", argv[1], ec.message);
  }
  bela::AppExecTarget ae;
  if (!bela::LookupAppExecLinkTarget(argv[1], ae)) {
    bela::FPrintF(stderr, L"unable lookup %s AppExecTarget\n", argv[1]);
    return 1;
  }
  bela::FPrintF(
      stdout,
      L"AppExecLink details:\nPackageID:  %s\nAppUserID:  %s\nTarget:     %s\n",
      ae.pkid, ae.appuserid, ae.target);
  return 0;
}