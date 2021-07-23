#include "scanner.hpp"
#include <bela/fs.hpp>

namespace git {
//
// GIT_ALTERNATE_OBJECT_DIRECTORIES

bool Scanner::Execute(const std::wstring_view gitdir, const Filter &filter, bela::error_code &ec) {
  if (!filter) {
    ec = bela::make_error_code(L"filter callback is nil");
    return false;
  }
  // L0
  auto objdir = bela::StringCat(gitdir, L"/objects");
  bela::fs::Finder finder;
  if (!finder.First(objdir, L"*", ec)) {
    return false;
  }
  return false;
}

} // namespace git