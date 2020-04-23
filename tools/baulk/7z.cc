//
#include <bela/path.hpp>
#include <bela/process.hpp>
#include "fs.hpp"

namespace baulk::sevenzip {
//
inline std::optional<std::wstring> lookup_sevenzip() {
  bela::error_code ec;
  auto parent = bela::ExecutableParent(ec);
  if (!parent) {
    return std::nullopt;
  }
  // baulk7z - A derivative version of 7-Zip-zstd maintained by baulk
  // contributors
  if (auto s7z = bela::StringCat(*parent, L"\\baulk7z.exe");
      bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  if (auto s7z = bela::StringCat(*parent, L"\\links\\7z.exe");
      !bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  std::wstring s7z;
  if (bela::ExecutableExistsInPath(L"7z.exe", s7z)) {
    return std::make_optional(std::move(s7z));
  }
  return std::nullopt;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec) {
  auto s7z = lookup_sevenzip();
  if (!s7z) {
    ec = bela::make_error_code(ERROR_NOT_FOUND, L"7z not install");
    return false;
  }
  bela::process::Process process;
  if (process.Execute(*s7z, L"e", L"-spf", L"-y", src,
                      bela::StringCat(L"-o", outdir)) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}
} // namespace baulk::sevenzip