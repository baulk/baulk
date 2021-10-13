///
/// baulk terminal vsenv initialize
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include <baulkenv.hpp>

namespace baulk::env {
// Visual studio instance
struct VisualStudioInstance {
  std::wstring installationPath;
  std::wstring installationVersion;
  std::wstring instanceId;
  std::wstring productId;
  bool isLaunchable{true};
  bool isPrerelease{false};
};

bool VisualStudioResultEncode(const std::string_view result, std::vector<VisualStudioInstance> &vsis,
                              bela::error_code &ec) {
  try {
    auto j0 = nlohmann::json::parse(result, nullptr, true, true);
    if (!j0.is_array() || j0.empty()) {
      ec = bela::make_error_code(bela::ErrGeneral, L"empty visual studio instance");
      return false;
    }
    for (auto &i : j0) {
      baulk::json::JsonAssignor ja(i);
      VisualStudioInstance vsi;
      vsi.installationPath = ja.get("installationPath");
      vsi.installationVersion = ja.get("installationVersion");
      vsi.instanceId = ja.get("instanceId");
      vsi.productId = ja.get("productId");
      vsi.isLaunchable = ja.boolean("isLaunchable");
      vsi.isPrerelease = ja.boolean("isPrerelease");
      vsis.emplace_back(std::move(vsi));
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::ToWide(e.what()));
    return false;
  }
  return true;
}

inline std::optional<std::wstring> LookupVisualCppVersion(std::wstring_view vsdir, bela::error_code &ec) {
  auto file = bela::StringCat(vsdir, L"/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
  return bela::io::ReadLine(file, ec);
}

// const fs::path vswhere_exe = program_files_32_bit / "Microsoft Visual Studio"
// / "Installer" / "vswhere.exe";
std::optional<std::wstring> LookupVsWhere() {
  constexpr std::wstring_view relativevswhere = L"/Microsoft Visual Studio/Installer/vswhere.exe";
  if (auto vswhere_exe = bela::StringCat(bela::GetEnv(L"ProgramFiles(x86)"), relativevswhere);
      bela::PathExists(vswhere_exe)) {
    return std::make_optional(std::move(vswhere_exe));
  }
  if (auto vswhere_exe = bela::StringCat(bela::GetEnv(L"ProgramFiles"), relativevswhere);
      bela::PathExists(vswhere_exe)) {
    return std::make_optional(std::move(vswhere_exe));
  }
  return std::nullopt;
}

std::optional<VisualStudioInstance> LookupVisualStudioInstance(bool preview, bela::error_code &ec) {
  auto vswhere_exe = LookupVsWhere();
  if (!vswhere_exe) {
    ec = bela::make_error_code(-1, L"vswhere not installed");
    return std::nullopt;
  }
  bela::process::Process process;
  // Force -utf8 convert to UTF8
  if (process.Capture(*vswhere_exe, L"-format", L"json", L"-utf8", L"-sort", L"-prerelease") != 0) {
    if (ec = process.ErrorCode(); !ec) {
      ec = bela::make_error_code(process.ExitCode(), L"vswhere exit with: ", process.ExitCode());
    }
    return std::nullopt;
  }
  std::vector<VisualStudioInstance> vsis;
  if (!VisualStudioResultEncode(process.Out(), vsis, ec)) {
    return std::nullopt;
  }
  if (vsis.empty()) {
    ec = bela::make_error_code(-1, L"visual studio not installed");
    return std::nullopt;
  }
  if (!preview) {
    for (auto &vsi : vsis) {
      if (!vsi.isPrerelease) {
        return std::make_optional(std::move(vsi));
      }
    }
    return std::make_optional(std::move(vsis[0]));
  }
  for (auto &vsi : vsis) {
    if (vsi.isPrerelease) {
      return std::make_optional(std::move(vsi));
    }
  }
  return std::make_optional(std::move(vsis[0]));
}

bool Searcher::FlushEnv() {
  if (!libs.empty()) {
    simulator.SetEnv(L"LIB", bela::JoinEnv(libs));
  }
  if (!includes.empty()) {
    simulator.SetEnv(L"INCLUDE", bela::JoinEnv(includes));
  }
  if (!libpaths.empty()) {
    simulator.SetEnv(L"LIBPATH", bela::JoinEnv(libpaths));
  }
  simulator.PathPushFront(std::move(paths));
  simulator.PathOrganize();
  return true;
}

std::optional<std::wstring> FrameworkDir() {
#ifdef _M_X64
  auto dir = bela::WindowsExpandEnv(LR"(%SystemRoot%\Microsoft.NET\Framework64)");
#else
  auto dir = bela::WindowsExpandEnv(LR"(%SystemRoot%\Microsoft.NET\Framework)");
#endif
  for (auto &p : std::filesystem::directory_iterator(dir)) {
    if (p.is_directory()) {
      auto dotnet = p.path().wstring();
      auto ngen = bela::StringCat(dotnet, L"\\ngen.exe");
      if (bela::PathExists(ngen)) {
        return std::make_optional(std::move(dotnet));
      }
    }
  }
  return std::nullopt;
}

bool SDKSearchVersion(std::wstring_view sdkroot, std::wstring_view sdkver, std::wstring &sdkversion) {
  auto dir = bela::StringCat(sdkroot, L"\\Include");
  for (auto &p : std::filesystem::directory_iterator(dir)) {
    auto filename = p.path().filename().wstring();
    if (bela::StartsWith(filename, sdkver)) {
      sdkversion = filename;
      return true;
    }
  }
  return true;
}

bool Searcher::InitializeWindowsKitEnv(bela::error_code &ec) {
  auto winsdk = baulk::regutils::LookupWindowsSDK(ec);
  if (!winsdk) {
    return false;
  }
  std::wstring sdkversion;
  if (!SDKSearchVersion(winsdk->InstallationFolder, winsdk->ProductVersion, sdkversion)) {
    ec = bela::make_error_code(bela::ErrGeneral, L"invalid sdk version");
    return false;
  }
  constexpr std::wstring_view incs[] = {L"\\um", L"\\ucrt", L"\\km", L"\\cppwinrt", L"\\shared", L"\\winrt"};
  for (auto i : incs) {
    JoinEnv(includes, winsdk->InstallationFolder, L"\\Include\\", sdkversion, i);
  }
  // libs
  JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, L"\\um\\", arch);
  JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, L"\\ucrt\\", arch);
  JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, L"\\km\\", arch);
  // Paths
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", HostArch);
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", sdkversion, L"\\", HostArch);
#ifdef _M_X64
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles(x86)"), LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\x64\)");
#else
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles"), LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\)");
#endif
  if (auto frameworkdir = FrameworkDir(); frameworkdir) {
    JoinEnv(paths, *frameworkdir);
  }
  // LIBPATHS
  auto unionmetadata = bela::StringCat(winsdk->InstallationFolder, L"\\UnionMetadata\\", sdkversion);
  JoinEnv(libpaths, unionmetadata);
  auto references = bela::StringCat(winsdk->InstallationFolder, L"\\References\\", sdkversion);
  JoinEnv(libpaths, references);
  // WindowsLibPath
  // C:\Program Files (x86)\Windows Kits\10\UnionMetadata\10.0.19041.0
  // C:\Program Files (x86)\Windows Kits\10\References\10.0.19041.0
  simulator.SetEnv(L"WindowsLibPath", bela::JoinEnv({unionmetadata, references}));
  simulator.SetEnv(L"WindowsSDKVersion", bela::StringCat(sdkversion, L"\\"));

  // ExtensionSdkDir
  if (auto ExtensionSdkDir = bela::WindowsExpandEnv(LR"(%ProgramFiles%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
      bela::PathExists(ExtensionSdkDir)) {
    simulator.SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  } else if (auto ExtensionSdkDir =
                 bela::WindowsExpandEnv(LR"(%ProgramFiles(x86)%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
             bela::PathExists(ExtensionSdkDir)) {
    simulator.SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  }
  return true;
}

bool Searcher::InitializeVisualStudioEnv(bool preview, bool clang, bela::error_code &ec) {
  auto vsi = LookupVisualStudioInstance(preview, ec);
  if (!vsi) {
    // Visual Studio not install
    return false;
  }
  auto vcver = LookupVisualCppVersion(vsi->installationPath, ec);
  if (!vcver) {
    return false;
  }
  std::vector<std::wstring_view> vv = bela::StrSplit(vsi->installationVersion, bela::ByChar('.'), bela::SkipEmpty());
  if (vv.size() > 2) {
    // VS160COMNTOOLS
    auto key = bela::StringCat(L"VS", vv[0], L"0COMNTOOLS");
    auto p = bela::StringCat(vsi->installationPath, LR"(\Common7\IDE\Tools\)");
  }
  // Libs
  JoinEnv(includes, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\include)");
  JoinEnv(includes, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\include)");
  // Libs
  JoinEnv(libs, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\lib\)", arch);
  JoinEnv(libs, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\)", arch);
  // Paths
  JoinEnv(paths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\bin\Host)", HostArch, L"\\", arch);
  // IDE tools
  JoinEnv(paths, vsi->installationPath, LR"(\Common7\IDE\VC\VCPackages)");
  JoinEnv(paths, vsi->installationPath, LR"(\Common7\IDE)");
  JoinEnv(paths, vsi->installationPath, LR"(\Common7\IDE\Tools)");
// Performance Tools
#ifdef _M_X64
  JoinEnv(paths, vsi->installationPath, LR"(Team Tools\Performance Tools\x64)");
#endif
  JoinEnv(paths, vsi->installationPath, LR"(Team Tools\Performance Tools)");
  //
  // Extension
  JoinEnv(paths, vsi->installationPath, LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin)");
  JoinEnv(paths, vsi->installationPath, LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja)");
#ifdef _M_X64
  JoinEnv(paths, vsi->installationPath, LR"(\MSBuild\Current\Bin\amd64)");
#else
  // msbuild
  JoinEnv(paths, vsi->installationPath, LR"(\MSBuild\Current\Bin)");
#endif
  if (clang) {
    // VC\Tools\Llvm\bin
#ifdef _M_X64
    if (auto llvmdir = bela::StringCat(vsi->installationPath, LR"(\VC\Tools\Llvm\x64\bin)");
        bela::PathExists(llvmdir)) {
      JoinForceEnv(paths, llvmdir);
    } else {
      JoinEnv(paths, vsi->installationPath, LR"(\VC\Tools\Llvm\bin)");
    }

#else
    JoinEnv(paths, vsi->installationPath, LR"(\VC\Tools\Llvm\bin)");
#endif
  }
  // add libpaths
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\lib\)", HostArch);
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\)", HostArch);
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\x86\store\references)");
  auto ifcpath = bela::StringCat(vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ifc\)", arch);
  if (bela::PathExists(ifcpath)) {
    simulator.SetEnv(L"IFCPATH", ifcpath);
  }
  simulator.SetEnv(L"VCIDEInstallDir", bela::StringCat(vsi->installationPath, LR"(Common7\IDE\VC)"));
  return true;
}
inline bool PathFileIsExists(std::wstring_view file) {
  auto at = GetFileAttributesW(file.data());
  return (INVALID_FILE_ATTRIBUTES != at && (at & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

std::optional<std::wstring> SearchBaulkRoot(bela::error_code &ec) {
  auto exepath = bela::ExecutableFinalPathParent(ec);
  if (!exepath) {
    return std::nullopt;
  }
  auto baulkexe = bela::StringCat(*exepath, L"\\baulk.exe");
  if (PathFileIsExists(baulkexe)) {
    return std::make_optional<std::wstring>(bela::DirName(*exepath));
  }
  std::wstring_view baulkroot(*exepath);
  for (size_t i = 0; i < 5; i++) {
    if (baulkroot == L".") {
      break;
    }
    auto baulkexe = bela::StringCat(baulkroot, L"\\bin\\baulk.exe");
    if (PathFileIsExists(baulkexe)) {
      return std::make_optional<std::wstring>(baulkroot);
    }
    baulkroot = bela::DirName(baulkroot);
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"unable found baulk.exe");
  return std::nullopt;
}

bool Searcher::InitializeBaulk(bela::error_code &ec) {
  auto bkroot = SearchBaulkRoot(ec);
  if (!bkroot) {
    return false;
  }
  baulkroot.assign(std::move(*bkroot));
  baulkbindir = bela::StringCat(baulkroot, L"\\bin");
  baulketc = bela::StringCat(baulkroot, L"\\bin\\etc");
  baulkvfs = bela::StringCat(baulkroot, L"\\bin\\vfs");
  JoinForceEnv(paths, baulkroot, L"\\bin");
  JoinForceEnv(paths, baulkroot, L"\\bin\\links");
  return true;
}

bool Searcher::InitializeGit(bool cleanup, bela::error_code &ec) {
  std::wstring git;
  if (bela::env::LookPath(L"git.exe", git, true)) {
    if (cleanup) {
      bela::PathStripName(git);
      JoinEnv(paths, git);
    }
    return true;
  }
  auto installPath = baulk::regutils::GitForWindowsInstallPath(ec);
  if (!installPath) {
    return false;
  }
  git = bela::StringCat(*installPath, L"\\cmd\\git.exe");
  if (!bela::PathExists(git)) {
    return false;
  }
  JoinEnv(paths, *installPath, L"\\cmd");
  return true;
}

} // namespace baulk::env
