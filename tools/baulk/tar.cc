/// ----- support tar.*
#include <string_view>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <regutils.hpp>
#include "decompress.hpp"
#include "fs.hpp"

namespace baulk::tar {

inline bool initialize_msys2tar(std::wstring &tar, bela::process::Process &p) {
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
    p.SetEnv(L"PATH", bela::StringCat(bela::GetEnv(L"PATH"), L";", *installPath,
                                      L"\\mingw64\\bin"));
    return true;
  }
#endif
  auto xz = bela::StringCat(*installPath, L"\\mingw32\\bin\\xz.exe");
  if (bela::PathExists(xz)) {
    p.SetEnv(L"PATH", bela::StringCat(bela::GetEnv(L"PATH"), L";", *installPath,
                                      L"\\mingw32\\bin"));
  }
  return true;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec) {
  if (!baulk::fs::MakeDir(outdir, ec)) {
    return false;
  }
  bela::process::Process process;
  std::wstring tar;
  if (!initialize_msys2tar(tar, process) &&
      !bela::ExecutableExistsInPath(L"tar.exe", tar)) {
    ec = bela::make_error_code(ERROR_NOT_FOUND, L"tar not install");
    return false;
  }
  process.Chdir(outdir);
  if (process.Execute(tar, L"-xvf", src) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}
} // namespace baulk::tar
