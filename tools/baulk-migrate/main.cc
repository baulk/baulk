/// migrate legacy mode to new Portable mode
#include <bela/terminal.hpp>
#include <baulk/vfs.hpp>

int wmain(int argc, wchar_t **argv) {
  bela::error_code ec;
  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-mirage InitializeFastPathFs error %s\n", ec.message);
    return false;
  }
  if (baulk::vfs::AppMode() != baulk::vfs::LegacyMode) {
    bela::FPrintF(stderr, L"baulk-migrate: current mode is %s not need migrate\n", baulk::vfs::AppMode());
    return 0;
  }

  return 0;
}