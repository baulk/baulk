///
#include <bela/simulator.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/process.hpp>
#include <filesystem>
#include "../regutils.hpp"
#include "json_utils.hpp"

namespace baulk::env {
#ifdef _M_X64
// Always build x64 binary
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x64"; // Hostx64 x64
#else
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x86"; // Hostx86 x86
#endif
namespace env_internal {

inline std::optional<std::wstring> lookup_vc_version(std::wstring_view vsdir, bela::error_code &ec) {
  auto file = bela::StringCat(vsdir, L"/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
  return bela::io::ReadLine(file, ec);
}

// const fs::path vswhere_exe = program_files_32_bit / "Microsoft Visual Studio"
// / "Installer" / "vswhere.exe";
inline std::optional<std::wstring> lookup_vswhere() {
  constexpr std::wstring_view relativevswhere = L"/Microsoft Visual Studio/Installer/vswhere.exe";
  constexpr std::wstring_view evv[] = {L"ProgramFiles(x86)", L"ProgramFiles"};
  for (auto e : evv) {
    if (auto vswhere_exe = bela::StringCat(bela::GetEnv(e), relativevswhere); bela::PathFileIsExists(vswhere_exe)) {
      return std::make_optional(std::move(vswhere_exe));
    }
  }
  return std::nullopt;
}

struct vs_instance {
  std::wstring path;
  std::wstring version;
  std::wstring instance_id;
  std::wstring product_id;
  bool is_launchable{true};
  bool is_prerelease{false};
};
using vs_instances = std::vector<vs_instance>;

std::optional<vs_instances> decode_vs_instances(const std::string_view text, bela::error_code &ec) {
  auto o = baulk::json::parse(text, ec);
  if (!o) {
    return std::nullopt;
  }
  if (!o->obj.is_array()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"empty visual studio instance");
    return std::nullopt;
  }
  vs_instances vss;
  for (auto &so : o->obj) {
    baulk::json::json_view jv(so);
    vs_instance vs{
        .path = jv.fetch("installationPath"),
        .version = jv.fetch("installationVersion"),
        .instance_id = jv.fetch("instanceId"),
        .product_id = jv.fetch("productId"),
        .is_launchable = jv.fetch("isLaunchable", false),
        .is_prerelease = jv.fetch("isPrerelease", false),
    };
    vss.emplace_back(std::move(vs));
  }
  if (vss.empty()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"empty visual studio instance");
    return std::nullopt;
  }
  return std::make_optional(std::move(vss));
}

inline std::optional<vs_instances> vs_instances_lookup(bela::error_code &ec) {
  auto vswhere = lookup_vswhere();
  if (!vswhere) {
    ec = bela::make_error_code(-1, L"vswhere not installed");
    return std::nullopt;
  }
  bela::process::Process process;
  // Force -utf8 convert to UTF8: include -prerelease
  if (process.Capture(*vswhere, L"-format", L"json", L"-utf8", L"-prerelease") == 0) {
    return decode_vs_instances(process.Out(), ec);
  }
  if (ec = process.ErrorCode(); !ec) {
    ec = bela::make_error_code(process.ExitCode(), L"vswhere exit with: ", process.ExitCode());
  }
  return std::nullopt;
}

inline bool sdk_search_version(std::wstring_view sdkroot, std::wstring_view sdkver, std::wstring &sdkversion) {
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

std::optional<std::wstring> dotnet_framwork_folder() {
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

using vector_t = std::vector<std::wstring>;

class vs_env_builder {
public:
  vs_env_builder(bela::env::Simulator *sm) : simulator(sm) {}
  vs_env_builder(const vs_env_builder &) = delete;
  vs_env_builder &operator=(const vs_env_builder &) = delete;
  bool initialize_windows_sdk(const std::wstring_view arch, bela::error_code &ec);
  bool initialize_vs_env(const vs_instance &vs, const std::wstring_view arch, bool usePreviewVS, bela::error_code &ec);
  void flush();

private:
  bela::env::Simulator *simulator{nullptr};
  vector_t paths;
  vector_t includes;
  vector_t libs;
  vector_t libpaths;
  bool JoinEnvInternal(vector_t &vec, std::wstring &&p) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  void JoinForceEnv(vector_t &vec, std::wstring_view p) { vec.emplace_back(std::wstring(p)); }
  void JoinForceEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    vec.emplace_back(bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view p) { return JoinEnvInternal(vec, std::wstring(p)); }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    return JoinEnvInternal(vec, bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c) {
    return JoinEnvInternal(vec, bela::StringCat(a, b, c));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d) {
    return JoinEnvInternal(vec, bela::StringCat(a, b, c, d));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d,
               Args... args) {
    return JoinEnvInternal(vec, bela::strings_internal::CatPieces({a, b, c, d, args...}));
  }
};

inline void vs_env_builder::flush() {
  if (!libs.empty()) {
    simulator->SetEnv(L"LIB", bela::JoinEnv(libs));
  }
  if (!includes.empty()) {
    simulator->SetEnv(L"INCLUDE", bela::JoinEnv(includes));
  }
  if (!libpaths.empty()) {
    simulator->SetEnv(L"LIBPATH", bela::JoinEnv(libpaths));
  }
  simulator->PathPushFront(std::move(paths));
  simulator->PathOrganize();
}

// initialize windows sdk
inline bool vs_env_builder::initialize_windows_sdk(const std::wstring_view arch, bela::error_code &ec) {
  auto winsdk = baulk::regutils::LookupWindowsSDK(ec);
  if (!winsdk) {
    return false;
  }
  std::wstring sdkversion;
  if (!sdk_search_version(winsdk->InstallationFolder, winsdk->ProductVersion, sdkversion)) {
    ec = bela::make_error_code(bela::ErrGeneral, L"invalid sdk version");
    return false;
  }
  constexpr std::wstring_view sdkincludes[] = {L"\\um", L"\\ucrt", L"\\km", L"\\cppwinrt", L"\\shared", L"\\winrt"};
  constexpr std::wstring_view sdklibs[] = {L"\\um\\", L"\\ucrt\\", L"\\km\\"};
  for (auto si : sdkincludes) {
    JoinEnv(includes, winsdk->InstallationFolder, L"\\Include\\", sdkversion, si);
  }
  for (auto sl : sdklibs) {
    JoinEnv(libs, winsdk->InstallationFolder, L"\\Lib\\", sdkversion, sl, arch);
  }
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", HostArch);
  JoinEnv(paths, winsdk->InstallationFolder, L"\\bin\\", sdkversion, L"\\", HostArch);
#ifdef _M_X64
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles(x86)"), LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\x64\)");
#else
  JoinEnv(paths, bela::GetEnv(L"ProgramFiles"), LR"(\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\)");
#endif
  if (auto frameworkdir = dotnet_framwork_folder(); frameworkdir) {
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
  simulator->SetEnv(L"WindowsLibPath", bela::JoinEnv({unionmetadata, references}));
  simulator->SetEnv(L"WindowsSDKVersion", bela::StringCat(sdkversion, L"\\"));

  // ExtensionSdkDir
  if (auto ExtensionSdkDir = bela::WindowsExpandEnv(LR"(%ProgramFiles%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
      bela::PathExists(ExtensionSdkDir)) {
    simulator->SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  } else if (auto ExtensionSdkDir =
                 bela::WindowsExpandEnv(LR"(%ProgramFiles(x86)%\Microsoft SDKs\Windows Kits\10\ExtensionSDKs)");
             bela::PathExists(ExtensionSdkDir)) {
    simulator->SetEnv(L"ExtensionSdkDir", ExtensionSdkDir);
  }
  return true;
}

// initialize vs env
inline bool vs_env_builder::initialize_vs_env(const vs_instance &vs, const std::wstring_view arch, bool usePreviewVS,
                                              bela::error_code &ec) {
  auto vcver = lookup_vc_version(vs.path, ec);
  if (!vcver) {
    return false;
  }
  std::vector<std::wstring_view> vv = bela::StrSplit(vs.version, bela::ByChar('.'), bela::SkipEmpty());
  if (vv.size() > 2) {
    // VS160COMNTOOLS
    auto key = bela::StringCat(L"VS", vv[0], L"0COMNTOOLS");
    auto p = bela::StringCat(vs.path, LR"(\Common7\IDE\Tools\)");
    simulator->SetEnv(key, p);
  }
  // Libs
  JoinEnv(includes, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\include)");
  JoinEnv(includes, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\include)");
  // Libs
  JoinEnv(libs, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\lib\)", arch);
  JoinEnv(libs, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\)", arch);
  // Paths
  JoinEnv(paths, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\bin\Host)", HostArch, L"\\", arch);

  constexpr std::wstring_view vsPathSuffix[] = {
      // IDE tools
      LR"(\Common7\IDE\VC\VCPackages)",
      LR"(\Common7\IDE)",
      LR"(\Common7\IDE\Tools)",
  // Performance Tools
#ifdef _M_X64
      LR"(\Team Tools\Performance Tools\x64)",
#endif
      LR"(\Team Tools\Performance Tools)",
      // Extension
      LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin)",
      LR"(\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja)",
      // MSBuild and LLVM
      LR"(\MSBuild\Current\bin\Roslyn)",
      LR"(\Common7\Tools\devinit)",
#ifdef _M_X64
      LR"(\MSBuild\Current\Bin\amd64)",
      LR"(\VC\Tools\Llvm\x64\bin)",
#else
      LR"(\MSBuild\Current\Bin)",
      LR"(\VC\Tools\Llvm\bin)",
#endif
      // LR"(\Common7\IDE\Extensions\Microsoft\IntelliCode\CLI)",
      LR"(\Common7\IDE\CommonExtensions\Microsoft\FSharp\Tools)",
      LR"(\Common7\IDE\VC\Linux\bin\ConnectionManagerExe)",
  };
  for (auto p : vsPathSuffix) {
    JoinEnv(paths, vs.path, p);
  }
  // add libpaths
  JoinEnv(libpaths, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ATLMFC\lib\)", HostArch);
  JoinEnv(libpaths, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\)", HostArch);
  JoinEnv(libpaths, vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\lib\x86\store\references)");
  auto ifcpath = bela::StringCat(vs.path, LR"(\VC\Tools\MSVC\)", *vcver, LR"(\ifc\)", arch);
  if (bela::PathExists(ifcpath)) {
    simulator->SetEnv(L"IFCPATH", ifcpath);
  }
  simulator->SetEnv(L"VCIDEInstallDir", bela::StringCat(vs.path, LR"(Common7\IDE\VC)"));
  return true;
}
} // namespace env_internal

// InitializeVisualStudioEnv initialize vs env
inline bool InitializeVisualStudioEnv(bela::env::Simulator &simulator, const std::wstring_view arch,
                                      const bool usePreviewVS, bela::error_code &ec) {
  auto vss = env_internal::vs_instances_lookup(ec);
  if (!vss) {
    return false;
  }
  if (vss->empty()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"empty visual studio instance");
    return false;
  }
  auto vs_matched = [&](const baulk::env::env_internal::vs_instance &vs_) { return vs_.is_prerelease == usePreviewVS; };
  auto vs = vss->begin();
  if (auto result = std::find_if(vss->begin(), vss->end(), vs_matched); result != vss->end()) {
    vs = result;
  }
  env_internal::vs_env_builder builder(&simulator);
  if (!builder.initialize_vs_env(*vs, arch, usePreviewVS, ec)) {
    return false;
  }
  if (!builder.initialize_windows_sdk(arch, ec)) {
    return false;
  }
  builder.flush();
  return true;
}
} // namespace baulk::env