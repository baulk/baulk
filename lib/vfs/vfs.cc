//
#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>

namespace baulk::vfs {
inline bool PathFileIsExists(std::wstring_view file) {
  auto at = GetFileAttributesW(file.data());
  return (INVALID_FILE_ATTRIBUTES != at && (at & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

std::optional<std::wstring> SearchBaulkPortableRoot(bela::error_code &ec) {
  auto exepath = bela::ExecutableFinalPathParent(ec);
  if (!exepath) {
    return std::nullopt;
  }
  auto baulkexe = bela::StringCat(*exepath, L"\\baulk.exe");
  if (PathFileIsExists(baulkexe)) {
    return std::make_optional<std::wstring>(bela::DirName(*exepath));
  }
  std::wstring_view portableRoot(*exepath);
  for (size_t i = 0; i < 5; i++) {
    if (portableRoot == L".") {
      break;
    }
    auto baulkexe = bela::StringCat(portableRoot, L"\\bin\\baulk.exe");
    if (PathFileIsExists(baulkexe)) {
      return std::make_optional<std::wstring>(portableRoot);
    }
    portableRoot = bela::DirName(portableRoot);
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"unable found baulk.exe");
  return std::nullopt;
}

bool PathMappingTable::InitializeFromPortable(bela::error_code &ec) {
  auto portableRoot = SearchBaulkPortableRoot(ec);
  if (!portableRoot) {
    return false;
  }
  return true;
}

bool PathMappingTable::InitializeFromLegacy(bela::error_code &ec) {
  auto portableRoot = SearchBaulkPortableRoot(ec);
  if (!portableRoot) {
    return false;
  }
  return true;
}

// $BAULK_VFS_ROOT/bin baulk cli
// $BAULK_VFS_ROOT/local/bin --> links
// $BAULK_VFS_ROOT/vfs
// $BAULK_VFS_ROOT/bucket
bool PathMappingTable::InitializeFromDesktopBridge(bela::error_code &ec) {
  //
  return true;
}

bool PathMappingTable::InitializeFromLocalAppData(bela::error_code &ec) {
  auto root = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\baulk");
  return true;
}

FsProvider &FsProvider::Instance() {
  static FsProvider inst;
  return inst;
}
bool FsProvider::Initialize(bela::error_code &ec) {
  if (initialized) {
    return true;
  }
  // TODO init
  initialized = true;
  return true;
}
const PathMappingTable &BaulkPathMappingTable() {
  //
  return FsProvider::Instance().PathTable();
}
} // namespace baulk::vfs