//
#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/env.hpp>
#include <bela/match.hpp>
#include <bela/subsitute.hpp>
#include <appmodel.h>
#include <filesystem>
#include <ShlObj.h>

namespace baulk::vfs {
std::optional<std::wstring> PackageFamilyName() {
  UINT32 len = 0;
  auto rc = GetCurrentPackageFamilyName(&len, nullptr);
  if (rc == APPMODEL_ERROR_NO_PACKAGE) {
    return std::nullopt;
  }
  std::wstring packageName;
  packageName.resize(len);
  if (rc = GetCurrentPackageFamilyName(&len, packageName.data()); rc != 0) {
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

std::optional<std::wstring> searchAppLocation(bool &appLocationIsFlat, bela::error_code &ec) {
  // GetModuleFileName returns the absolute path when launched from the application alias
  auto parent = bela::ExecutableFinalPathParent(ec);
  if (!parent) {
    return std::nullopt;
  }
  auto baulExe = bela::StringCat(*parent, L"\\baulk.exe");
  if (bela::PathFileIsExists(baulExe)) {
    if (appLocationIsFlat = !bela::EqualsIgnoreCase(bela::BaseName(*parent), L"bin"); appLocationIsFlat) {
      return std::make_optional<std::wstring>(std::move(*parent));
    }
    return std::make_optional<std::wstring>(bela::DirName(*parent));
  }
  std::wstring_view appLocation(*parent);
  for (size_t i = 0; i < 5; i++) {
    if (appLocation == L".") {
      break;
    }
    auto baulkexe = bela::StringCat(appLocation, L"\\bin\\baulk.exe");
    if (bela::PathFileIsExists(baulkexe)) {
      return std::make_optional<std::wstring>(appLocation);
    }
    appLocation = bela::DirName(appLocation);
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

constexpr std::wstring_view envTemplate = LR"({
    // Do not modify, baulk relies on this file to select the operating mode  
    "mode": "$0"
})";

bool PathFs::NewFsPaths(bela::error_code &ec) {
  std::error_code e;
  if (std::filesystem::create_directories(table.basePath, e); e) {
    ec = bela::make_error_code_from_std(e, L"create_directories: ");
    return false;
  }
  auto baulkEnv = bela::StringCat(table.basePath, L"\\baulk.env");
  if (!bela::PathFileIsExists(baulkEnv)) {
    if (!bela::io::WriteText(baulkEnv, bela::Substitute(envTemplate, mode), ec)) {
      return false;
    }
  }
  const std::filesystem::path paths[] = {
      // Paths
      table.etc,      // etc
      table.appData,  // appdata
      table.vfs,      // vfs
      table.packages, // packages
      table.temp,     // temp
      table.locks,    // locks
      table.buckets,  // buckets
      table.appLinks, // local/bin
  };
  for (const auto &p : paths) {
    if (!std::filesystem::exists(p, e)) {
      (void)std::filesystem::create_directories(p, e);
    }
  }
  return true;
}

bool PathFs::InitializeInternal(bela::error_code &ec) {
  auto appLocation = searchAppLocation(table.appLocationFlat, ec);
  if (!appLocation) {
    return false;
  }
  table.appLocation.assign(std::move(*appLocation));
  if (IsPackaged()) {
    mode = L"Packaged";
    return table.InitializeFromPackaged(ec);
  }
  auto envfile = bela::StringCat(table.appLocation, L"\\baulk.env");
  if (!bela::PathFileIsExists(envfile)) {
    mode = L"Legacy";
    return table.InitializeFromLegacy(ec);
  }
  if (!InitializeBaulkEnv(envfile, ec)) {
    return false;
  }
  if (bela::EqualsIgnoreCase(mode, L"User")) {
    return table.InitializeFromLocalAppData(ec);
  }
  if (bela::EqualsIgnoreCase(mode, L"System")) {
    return table.InitializeFromSystemAppData(ec);
  }

  if (bela::EndsWithIgnoreCase(mode, L"Portable")) {
    return table.InitializeFromPortable(ec);
  }
  return table.InitializeFromLegacy(ec);
}

} // namespace baulk::vfs