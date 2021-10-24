#include <bela/path.hpp>
#include "baulk.hpp"
#include "decompress.hpp"
#include <baulk/fs.hpp>

namespace baulk {

namespace standard {
bool Regularize(std::wstring_view path) {
  bela::error_code ec;
  // TODO some zip code
  return baulk::fs::MakeFlattened(path, path, ec);
}
} // namespace standard

namespace exe {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  if (!baulk::fs::MakeDir(dest, ec)) {
    return false;
  }
  auto fn = baulk::fs::FileName(src);
  auto newfile = bela::StringCat(dest, L"\\", fn);
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