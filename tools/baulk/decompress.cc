#include <bela/path.hpp>
#include "baulk.hpp"
#include "decompress.hpp"
#include "fs.hpp"

namespace baulk {

namespace standard {
bool Regularize(std::wstring_view path) {
  bela::error_code ec;
  // TODO some zip code
  return baulk::fs::FlatPackageInitialize(path, path, ec);
}
} // namespace standard

namespace exe {
bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  if (!baulk::fs::MakeDir(outdir, ec)) {
    return false;
  }
  auto fn = baulk::fs::FileName(outdir);
  auto extension = std::filesystem::path(src).extension().wstring();
  auto newfile = bela::StringCat(outdir, L"\\", fn, extension);
  auto newfileold = bela::StringCat(newfile, L".old");
  if (bela::PathExists(newfile)) {
    if (MoveFileW(newfile.data(), newfileold.data()) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
  }
  if (MoveFileW(src.data(), newfile.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

bool Regularize(std::wstring_view path) {
  std::error_code ec;
  for (auto &p : std::filesystem::directory_iterator(path)) {
    if (p.path().extension() == L".old") {
      std::filesystem::remove_all(p.path(), ec);
    }
  }
  return true;
}
} // namespace exe

} // namespace baulk