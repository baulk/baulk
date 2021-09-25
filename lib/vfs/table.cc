#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>

namespace baulk::vfs::vfs_internal {
std::optional<std::wstring> SearchBaulkPortableRoot(bela::error_code &ec) {
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

// Protable (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPortable(bela::error_code &ec) {
  auto portableRoot = SearchBaulkPortableRoot(ec);
  if (!portableRoot) {
    return false;
  }
  return true;
}

// Legacy Install (baulk <=3.0)
bool FsRedirectionTable::InitializeFromLegacy(bela::error_code &ec) {
  auto portableRoot = SearchBaulkPortableRoot(ec);
  if (!portableRoot) {
    return false;
  }
  return true;
}

// Windows Store (baulk >=4.0)
bool FsRedirectionTable::InitializeFromDesktopBridge(bela::error_code &ec) {
  //
  return true;
}

// User Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromLocalAppData(bela::error_code &ec) {
  auto root = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\baulk");
  return true;
}

// System Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromSystemAppData(bela::error_code &ec) {
  //
  return true;
}
} // namespace baulk::vfs::vfs_internal