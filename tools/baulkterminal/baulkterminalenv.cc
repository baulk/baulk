/// baulk terminal vsenv initialize
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include "baulkterminal.hpp"
#include "fs.hpp"

namespace baulkterminal {
#ifdef _M_X64
// Always build x64 binary
[[maybe_unused]] constexpr std::wstring_view arch = L"x64"; // Hostx64 x64
#else
[[maybe_unused]] constexpr std::wstring_view arch = L"x86"; // Hostx86 x86
#endif

struct VisualStudioInstance {
  std::wstring installationPath;
  std::wstring installationVersion;
  std::wstring instanceId;
  std::wstring productId;
  bool isLaunchable{true};
  bool isPrerelease{false};
  bool Encode(const std::string_view result, bela::error_code &ec) {
    try {
      auto j0 = nlohmann::json::parse(result);
      if (!j0.is_array() || j0.empty()) {
        ec = bela::make_error_code(1, L"empty visual studio instance");
        return false;
      }
      baulk::json::JsonAssignor ja(j0[0]);
      installationPath = ja.get("installationPath");
      installationVersion = ja.get("installationVersion");
      instanceId = ja.get("instanceId");
      productId = ja.get("productId");
      isLaunchable = ja.boolean("isLaunchable");
      isPrerelease = ja.boolean("isPrerelease");
    } catch (const std::exception &e) {
      ec = bela::make_error_code(1, bela::ToWide(e.what()));
      return false;
    }
    return true;
  }
};

inline std::optional<std::wstring>
LookupVisualCppVersion(std::wstring_view vsdir, bela::error_code &ec) {
  auto file = bela::StringCat(
      vsdir, L"/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
  return bela::io::ReadLine(file, ec);
}

// const fs::path vswhere_exe = program_files_32_bit / "Microsoft Visual Studio"
// / "Installer" / "vswhere.exe";
std::optional<std::wstring> LookupVsWhere() {
  constexpr std::wstring_view relativevswhere =
      L"/Microsoft Visual Studio/Installer/vswhere.exe";
  if (auto vswhere_exe =
          bela::StringCat(bela::GetEnv(L"ProgramFiles(x86)"), relativevswhere);
      bela::PathExists(vswhere_exe)) {
    return std::make_optional(std::move(vswhere_exe));
  }
  if (auto vswhere_exe =
          bela::StringCat(bela::GetEnv(L"ProgramFiles"), relativevswhere);
      bela::PathExists(vswhere_exe)) {
    return std::make_optional(std::move(vswhere_exe));
  }
  return std::nullopt;
}

std::optional<VisualStudioInstance>
LookupVisualStudioInstance(bela::error_code &ec) {
  auto vswhere_exe = LookupVsWhere();
  if (!vswhere_exe) {
    ec = bela::make_error_code(-1, L"vswhere not installed");
    return std::nullopt;
  }
  bela::process::Process process;
  // Force -utf8 convert to UTF8
  if (process.Capture(*vswhere_exe, L"-format", L"json", L"-utf8") != 0) {
    if (ec = process.ErrorCode(); !ec) {
      ec = bela::make_error_code(process.ExitCode(), L"vswhere exit with: ",
                                 process.ExitCode());
    }
    return std::nullopt;
  }
  VisualStudioInstance vsi;
  if (!vsi.Encode(process.Out(), ec)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(vsi));
}
// HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft
// SDKs\Windows\v10.0 InstallationFolder ProductVersion

struct Searcher {
  Searcher(bela::env::Derivator &dev_) : dev{dev_} {}
  using vector_t = std::vector<std::wstring>;
  bela::env::Derivator &dev;
  vector_t paths;
  vector_t libs;
  vector_t includes;
  vector_t libpaths;
  bool JoinEnvInternal(vector_t &vec, std::wstring &&p) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  bool JoinEnv(vector_t &vec, std::wstring_view p) {
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    auto p = bela::StringCat(a, b);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b,
               std::wstring_view c) {
    auto p = bela::StringCat(a, b, c);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b,
               std::wstring_view c, std::wstring_view d) {
    auto p = bela::StringCat(a, b, c, d);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b,
               std::wstring_view c, std::wstring_view d, Args... args) {
    auto p = bela::strings_internal::CatPieces({a, b, c, d, args...});
    return JoinEnvInternal(vec, std::wstring(p));
  }
  std::wstring CleanupEnv() {
    if (!libs.empty()) {
      dev.SetEnv(L"LIB", bela::env::JoinEnv(libs));
    }
    if (!includes.empty()) {
      dev.SetEnv(L"INCLUDE", bela::env::JoinEnv(includes));
    }
    if (!libpaths.empty()) {
      dev.SetEnv(L"LIBPATH", bela::env::JoinEnv(libpaths));
    }
    return dev.CleanupEnv(bela::env::JoinEnv(paths));
  }

  std::wstring MakeEnv() {
    if (!libs.empty()) {
      dev.SetEnv(L"LIB", bela::env::JoinEnv(libs));
    }
    if (!includes.empty()) {
      dev.SetEnv(L"INCLUDE", bela::env::JoinEnv(includes));
    }
    if (!libpaths.empty()) {
      dev.SetEnv(L"LIBPATH", bela::env::JoinEnv(libpaths));
    }
    auto oldpath = bela::GetEnv(L"path");
    std::vector<std::wstring_view> ov = bela::StrSplit(
        oldpath, bela::ByChar(bela::env::Separator), bela::SkipEmpty());
    auto newpath =
        bela::StringCat(bela::env::JoinEnv(paths), bela::env::Separators,
                        bela::env::JoinEnv(ov));
    dev.SetEnv(L"path", newpath);
    return dev.MakeEnv();
  }

  bool InitializeWindowsKitEnv(bela::error_code &ec);
  bool InitializeVisualStudioEnv(bool clang, bela::error_code &ec);
  bool InitializeBaulk(bela::error_code &ec);
  bool InitializeGit(bool cleanup, bela::error_code &ec);
};

std::optional<std::wstring> FrameworkDir() {
#ifdef _M_X64
  auto dir = bela::ExpandEnv(LR"(%SystemRoot%\Microsoft.NET\Framework64)");
#else
  auto dir = bela::ExpandEnv(LR"(%SystemRoot%\Microsoft.NET\Framework)");
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

bool SDKSearchVersion(std::wstring_view sdkroot, std::wstring_view sdkver,
                      std::wstring &sdkversion) {
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
  if (!SDKSearchVersion(winsdk->InstallationFolder, winsdk->ProductVersion,
                        sdkversion)) {
    ec = bela::make_error_code(1, L"invalid sdk version");
    return false;
  }
  constexpr std::wstring_view incs[] = {L"\\um", L"\\ucrt", L"\\cppwinrt",
                                        L"\\shared", L"\\winrt"};
  for (auto i : incs) {
    JoinEnv(includes, winsdk->InstallationFolder, L"\\Include\\", sdkversion,
            i);
  }
  // libs
  JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, L"\\um\\",
          arch);
  JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, L"\\ucrt\\",
          arch);
  // Paths
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", arch);
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", sdkversion, L"\\",
          arch);
#ifdef _M_X64
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles(x86)"),
          LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\x64\)");
#else
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles"),
          LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\)");
#endif
  if (auto frameworkdir = FrameworkDir(); frameworkdir) {
    JoinEnv(paths, *frameworkdir);
  }
  // LIBPATHS
  auto unionmetadata = bela::StringCat(winsdk->InstallationFolder,
                                       L"\\UnionMetadata\\", sdkversion);
  JoinEnv(libpaths, unionmetadata);
  auto references = bela::StringCat(winsdk->InstallationFolder,
                                    L"\\References\\", sdkversion);
  JoinEnv(libpaths, references);
  // WindowsLibPath
  // C:\Program Files (x86)\Windows Kits\10\UnionMetadata\10.0.19041.0
  // C:\Program Files (x86)\Windows Kits\10\References\10.0.19041.0
  dev.SetEnv(L"WindowsLibPath",
             bela::env::JoinEnv({unionmetadata, references}));
  dev.SetEnv(L"WindowsSDKVersion", bela::StringCat(sdkversion, L"\\"));

  // ExtensionSdkDir
  if (auto ExtensionSdkDir = bela::ExpandEnv(
          LR"(%ProgramFiles%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
      bela::PathExists(ExtensionSdkDir)) {
    dev.SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  } else if (
      auto ExtensionSdkDir = bela::ExpandEnv(
          LR"(%ProgramFiles(x86)%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
      bela::PathExists(ExtensionSdkDir)) {
    dev.SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  }
  return true;
}

bool Searcher::InitializeVisualStudioEnv(bool clang, bela::error_code &ec) {
  auto vsi = LookupVisualStudioInstance(ec);
  if (!vsi) {
    // Visual Studio not install
    return false;
  }
  auto vcver = LookupVisualCppVersion(vsi->installationPath, ec);
  if (!vcver) {
    return false;
  }
  std::vector<std::wstring_view> vv = bela::StrSplit(
      vsi->installationVersion, bela::ByChar('.'), bela::SkipEmpty());
  if (vv.size() > 2) {
    // VS160COMNTOOLS
    auto key = bela::StringCat(L"VS", vv[0], L"0COMNTOOLS");
    auto p = bela::StringCat(vsi->installationPath, LR"(\Common7\IDE\Tools\)");
  }
  // Libs
  JoinEnv(includes, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\ATLMFC\include)");
  JoinEnv(includes, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\include)");
  // Libs
  JoinEnv(libs, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\ATLMFC\lib\)", arch);
  JoinEnv(libs, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\lib\)", arch);
  // Paths
  JoinEnv(paths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\bin\Host)", arch, L"\\", arch);
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
  JoinEnv(paths, vsi->installationPath,
          LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin)");
  JoinEnv(paths, vsi->installationPath,
          LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja)");
#ifdef _M_X64
  JoinEnv(paths, vsi->installationPath, LR"(\MSBuild\Current\Bin\amd64)");
#else
  // msbuild
  JoinEnv(paths, vsi->installationPath, LR"(\MSBuild\Current\Bin)");
#endif
  if (clang) {
    // VC\Tools\Llvm\bin
    JoinEnv(paths, vsi->installationPath, LR"(\VC\Tools\Llvm\bin)");
  }
  // add libpaths
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\ATLMFC\lib\)", arch);
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\lib\)", arch);
  JoinEnv(libpaths, vsi->installationPath, LR"(\VC\Tools\MSVC\)", *vcver,
          LR"(\lib\x86\store\references)");
  auto ifcpath = bela::StringCat(vsi->installationPath, LR"(\VC\Tools\MSVC\)",
                                 *vcver, LR"(\ifc\)", arch);
  if (bela::PathExists(ifcpath)) {
    dev.SetEnv(L"IFCPATH", ifcpath);
  }
  dev.SetEnv(L"VCIDEInstallDir",
             bela::StringCat(vsi->installationPath, LR"(Common7\IDE\VC)"));
  return true;
}

bool Searcher::InitializeBaulk(bela::error_code &ec) {
  auto exepath = bela::ExecutableFinalPathParent(ec);
  if (!exepath) {
    return false;
  }
  auto baulkexe = bela::StringCat(*exepath, L"\\baulk.exe");
  if (bela::PathExists(baulkexe)) {
    JoinEnv(paths, *exepath);
    JoinEnv(paths, *exepath, L"\\links");
    return true;
  }
  std::wstring baulkroot(*exepath);
  for (size_t i = 0; i < 5; i++) {
    auto baulkexe = bela::StringCat(baulkroot, L"\\bin\\baulk.exe");
    if (bela::PathExists(baulkexe)) {
      JoinEnv(paths, baulkroot, L"\\bin");
      JoinEnv(paths, baulkroot, L"\\bin\\links");
      return true;
    }
    bela::PathStripName(baulkroot);
  }
  ec = bela::make_error_code(1, L"unable found baulk.exe");
  return false;
}
bool Searcher::InitializeGit(bool cleanup, bela::error_code &ec) {
  std::wstring git;
  if (bela::ExecutableExistsInPath(L"git.exe", git)) {
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

std::wstring Executor::MakeShell() const {
  if (shell.empty() || bela::EqualsIgnoreCase(L"pwsh", shell)) {
    if (auto pwsh = PwshExePath(); !pwsh.empty()) {
      return pwsh;
    }
  }
  if (bela::EqualsIgnoreCase(L"bash", shell)) {
    // git for windows
    bela::error_code ec;
    if (auto gw = baulk::regutils::GitForWindowsInstallPath(ec); gw) {
      if (auto bash = bela::StringCat(*gw, L"\\bin\\bash.exe");
          bela::PathExists(bash)) {
        return bash;
      }
    }
  }
  if (bela::EqualsIgnoreCase(L"wsl", shell)) {
    return L"wsl.exe";
  }
  if (!shell.empty() && bela::PathExists(shell)) {
    return shell;
  }
  return L"cmd.exe";
}

bool Executor::InitializeBaulkEnv(bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, manifest.data(), L"rb"); eo != 0) {
    ec = bela::make_stdc_error_code(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto obj = nlohmann::json::parse(fd);
    if (auto it = obj.find("env"); it != obj.end()) {
      for (const auto &kv : it.value().items()) {
        auto value = bela::ToWide(kv.value().get<std::string_view>());
        dev.SetEnv(bela::ToWide(kv.key()), bela::ExpandEnv(value), true);
      }
    }
    baulk::json::JsonAssignor ja(obj);
    if (!clang.initialized) {
      clang = ja.boolean("clang");
    }
    if (!usevs.initialized) {
      clang = ja.boolean("usevs");
    }
    if (!cleanup.initialized) {
      cleanup = ja.boolean("cleanup");
    }
    if (!conhost.initialized) {
      conhost = ja.boolean("conshot");
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  return true;
}

template <size_t Len = 256> std::wstring GetCwd() {
  std::wstring s;
  s.resize(Len);
  auto len = GetCurrentDirectoryW(Len, s.data());
  if (len == 0) {
    return L"";
  }
  if (len < Len) {
    s.resize(len);
    return s;
  }
  s.resize(len);
  auto nlen = GetCurrentDirectoryW(len, s.data());
  if (nlen == 0 || nlen > len) {
    return L"";
  }
  s.resize(nlen);
  return s;
}

bool Executor::PrepareEnv(bela::error_code &ec) {
  if (!manifest.empty() && !InitializeBaulkEnv(ec)) {
    return false;
  }
  if (cwd.empty()) {
    cwd = GetCwd();
  }
  return true;
}

std::optional<std::wstring> Executor::MakeEnv(bela::error_code &ec) {
  Searcher searcher(dev);
  if (!searcher.InitializeBaulk(ec)) {
    return std::nullopt;
  }
  searcher.InitializeGit(cleanup(), ec);
  if (usevs) {
    if (!searcher.InitializeVisualStudioEnv(clang(), ec)) {
      return std::nullopt;
    }
    if (!searcher.InitializeWindowsKitEnv(ec)) {
      return std::nullopt;
    }
  }
  if (cleanup) {
    return std::make_optional(searcher.CleanupEnv());
  }
  return std::make_optional(searcher.MakeEnv());
}

} // namespace baulkterminal