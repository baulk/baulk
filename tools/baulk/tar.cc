/// ----- support tar.*
#include <string_view>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/simulator.hpp>
#include <regutils.hpp>
#include "decompress.hpp"
#include "fs.hpp"

namespace baulk::tar {

inline bool initialize_baulktar(std::wstring &tar) {
  bela::error_code ec;
  auto parent = bela::ExecutableParent(ec);
  if (!parent) {
    return false;
  }
  // rethink tar
  if (tar = bela::StringCat(*parent, L"\\links\\baulktar.exe"); bela::PathExists(tar)) {
    return true;
  }
  // // Full UTF-8 support
  // if (tar = bela::StringCat(*parent, L"\\links\\wintar.exe");
  //     bela::PathExists(tar)) {
  //   return true;
  // }
  // libarchive bsdtar, baulk build
  if (tar = bela::StringCat(*parent, L"\\links\\bsdtar.exe"); bela::PathExists(tar)) {
    return true;
  }
  return false;
}

inline bool initialize_msys2tar(std::wstring &tar, bela::env::Simulator &simulator) {
  bela::error_code ec;
  auto installPath = baulk::regutils::GitForWindowsInstallPath(ec);
  if (!installPath) {
    return false;
  }
  bela::StrAppend(&tar, *installPath, L"\\user\\bin\\tar.exe");
  if (!bela::PathExists(tar)) {
    return false;
  }
  // tar.xz support
#ifdef _M_X64
  auto xz64 = bela::StringCat(*installPath, L"\\mingw64\\bin\\xz.exe");
  if (bela::PathExists(xz64)) {
    simulator.PathAppend(bela::StringCat(*installPath, L"\\mingw64\\bin"));
    return true;
  }
#endif
  auto xz = bela::StringCat(*installPath, L"\\mingw32\\bin\\xz.exe");
  if (bela::PathExists(xz)) {
    simulator.PathAppend(bela::StringCat(*installPath, L"\\mingw64\\bin"));
  }
  return true;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  if (!baulk::fs::MakeDir(outdir, ec)) {
    return false;
  }
  bela::env::Simulator simulator;
  simulator.InitializeEnv();
  std::wstring tar;
  if (!initialize_baulktar(tar) && !initialize_msys2tar(tar, simulator) &&
      !bela::env::ExecutableExistsInPath(L"tar.exe", tar)) {
    ec = bela::make_error_code(ERROR_NOT_FOUND, L"tar not install");
    return false;
  }
  bela::process::Process process(&simulator);
  process.Chdir(outdir);
  if (process.Execute(tar, L"-xvf", src) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}
} // namespace baulk::tar
