///
#include <bela/stdwriter.hpp>
#include <bela/path.hpp>
// test\appexeclink\appexeclink_test.exe
// C:\Users\$Username\AppData\Local\Microsoft\WindowsApps\wt.exe
int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s appexeclink\n", argv[0]);
    return 1;
  }
  bela::AppExecTarget ae;
  if (!bela::LookupAppExecLinkTarget(argv[1], ae)) {
    bela::FPrintF(stderr, L"unable lookup %s AppExecTarget\n", argv[1]);
    return 1;
  }
  bela::FPrintF(
      stderr,
      L"AppExecLink details:\nPackageID:  %s\nAppUserID:  %s\nTarget:     %s\n",
      ae.pkid, ae.appuserid, ae.target);
  return 0;
}