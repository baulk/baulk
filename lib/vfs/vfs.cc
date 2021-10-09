//
#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/env.hpp>
#include <bela/match.hpp>
#include <appmodel.h>
#include <filesystem>
#include <ShlObj.h>

namespace baulk::vfs {
std::optional<std::wstring> PackageFamilyName() {
  UINT32 len = 0;
  auto rc = GetCurrentPackageFamilyName(&len, 0);
  if (rc == APPMODEL_ERROR_NO_PACKAGE) {
    return std::nullopt;
  }
  std::wstring packageName;
  packageName.resize(len);
  if (rc = GetCurrentPackageFamilyName(&len, packageName.data()) != 0) {
    return std::nullopt;
  }
  packageName.resize(len);
  return std::make_optional(std::move(packageName));
}

bool IsPackaged() {
  static const bool isPackaged = []() -> bool { return !!PackageFamilyName(); }();
  return isPackaged;
}

static constexpr std::wstring_view UnpackagedFolderName{L"\\Baulk"};
std::wstring_view GetAppBasePath() {
  static std::wstring basePath = []() {
    PWSTR localAppDataFolder{nullptr};
    auto closer = bela::finally([&] {
      if (localAppDataFolder != nullptr) {
        CoTaskMemFree(localAppDataFolder);
      }
    });
    std::wstring basePath;
    // KF_FLAG_FORCE_APP_DATA_REDIRECTION, when engaged, causes SHGet... to return
    // the new AppModel paths (Packages/xxx/RoamingState, etc.) for standard path requests.
    // Using this flag allows us to avoid Windows.Storage.ApplicationData completely.
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_FORCE_APP_DATA_REDIRECTION, nullptr, &localAppDataFolder) !=
        S_OK) {
      basePath = bela::WindowsExpandEnv(L"%LOCALAPPDATA%");
    } else {
      basePath = localAppDataFolder;
    }
    if (!IsPackaged()) {
      basePath += UnpackagedFolderName;
    }
    // Create the directory if it doesn't exist
    std::filesystem::create_directories(basePath);
    return basePath;
  }();
  return basePath;
}

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
  if (IsPackaged()) {
    fsmodel = L"DesktopBridge";
    return table.InitializeFromDesktopBridge(ec);
  }
  auto baulkRoot = searchBaulkPortableRoot(ec);
  if (!baulkRoot) {
    return false;
  }
  auto envfile = bela::StringCat(*baulkRoot, L"\\baulk.env");
  if (!bela::PathFileIsExists(envfile)) {
    fsmodel = L"Legacy";
    return table.InitializeFromLegacy(*baulkRoot, ec);
  }
  if (!InitializeBaulkEnv(envfile, ec)) {
    return false;
  }
  if (bela::EqualsIgnoreCase(fsmodel, L"User")) {
    return table.InitializeFromLocalAppData(ec);
  }
  if (bela::EqualsIgnoreCase(fsmodel, L"System")) {
    return table.InitializeFromSystemAppData(ec);
  }
  if (bela::EndsWithIgnoreCase(fsmodel, L"Portable")) {
    return table.InitializeFromPortable(*baulkRoot, ec);
  }
  return table.InitializeFromLegacy(*baulkRoot, ec);
}

} // namespace baulk::vfs