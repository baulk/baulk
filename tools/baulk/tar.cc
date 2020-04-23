/// ----- support tar.*
#include <string_view>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include "decompress.hpp"
#include "fs.hpp"

namespace baulk::tar {

inline std::optional<std::wstring> lookup_tarexe() {
  std::wstring tar;
  if (bela::ExecutableExistsInPath(L"tar.exe", tar)) {
    return std::make_optional(std::move(tar));
  }
  return std::nullopt;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec) {
  auto tar = lookup_tarexe();
  if (!tar) {
    ec = bela::make_error_code(ERROR_NOT_FOUND, L" tar not install");
    return false;
  }
  if (!baulk::fs::MakeDir(outdir, ec)) {
    return false;
  }
  bela::process::Process process;
  process.Chdir(outdir);
  if (process.Execute(*tar, L"-xvf", src) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}
} // namespace baulk::tar
