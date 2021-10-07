//
#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/env.hpp>
#include <appmodel.h>
#include <toml.hpp>

namespace baulk::vfs {
std::optional<std::wstring> searchBaulkPortableRoot(bela::error_code &ec) {
  auto exepath = bela::ExecutableFinalPathParent(ec);
  if (!exepath) {
    return std::nullopt;
  }
  auto baulkexe = bela::StringCat(*exepath, L"\\baulk.exe");
  if (bela::PathFileIsExists(baulkexe)) {
    return std::make_optional<std::wstring>(bela::DirName(*exepath));
  }
  std::wstring_view portableRoot(*exepath);
  for (size_t i = 0; i < 5; i++) {
    if (portableRoot == L".") {
      break;
    }
    auto baulkexe = bela::StringCat(portableRoot, L"\\bin\\baulk.exe");
    if (bela::PathFileIsExists(baulkexe)) {
      return std::make_optional<std::wstring>(portableRoot);
    }
    portableRoot = bela::DirName(portableRoot);
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"unable found baulk.exe");
  return std::nullopt;
}

PathFs &PathFs::Instance() {
  static PathFs inst;
  return inst;
}

bool PathFs::Initialize(bela::error_code &ec) {
  bool result = true;
  std::call_once(initialized, [&, this] {
    //
    result = InitializeInternal(ec);
  });
  return result;
}

bool PathFs::InitializeInternal(bela::error_code &ec) {
  //
  return false;
}

} // namespace baulk::vfs