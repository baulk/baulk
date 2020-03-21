/// baulk terminal vsenv initialize
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include "baulkterminal.hpp"

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

std::optional<std::wstring> LookupVisualCppVersion(std::wstring_view vsdir,
                                                   bela::error_code &ec) {
  auto file = bela::StringCat(
      vsdir, L"/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
  auto line = bela::io::ReadLine(file, ec);
  if (!line) {
    return std::nullopt;
  }
  return std::make_optional(std::move(*line));
}

// const fs::path vswhere_exe = program_files_32_bit / "Microsoft Visual Studio"
// / "Installer" / "vswhere.exe";

std::optional<std::wstring> LookupVsWhere() {
  auto vswhere_exe =
      bela::StringCat(bela::GetEnv(L"ProgramFiles(x86)"),
                      L"/Microsoft Visual Studio/Installer/vswhere.exe");
  if (bela::PathExists(vswhere_exe)) {
    return std::make_optional(std::move(vswhere_exe));
  }
  vswhere_exe =
      bela::StringCat(bela::GetEnv(L"ProgramFiles"),
                      L"/Microsoft Visual Studio/Installer/vswhere.exe");
  if (bela::PathExists(vswhere_exe)) {
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
  using vector_t = std::vector<std::wstring>;
  vector_t paths;
  vector_t libs;
  vector_t includes;
  vector_t libpaths;
  bool InitializeWindowsKitEnv(bela::error_code &ec);
  bool InitializeVisualStudioEnv(bela::error_code &ec);
  bool TestJoin(std::wstring &&p, vector_t &vec) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  std::wstring CleanupEnv() const {
    bela::env::Derivator dev;
    dev.SetEnv(L"LIB", bela::env::JoinEnv(libs));
    dev.SetEnv(L"INCLUDE", bela::env::JoinEnv(includes));
    dev.SetEnv(L"LIBPATH", bela::env::JoinEnv(libpaths));
    // dev.SetEnv(L"Path", bela::env::InsertEnv(L"Path", paths));
    return dev.CleanupEnv(bela::env::JoinEnv(paths));
  }
};

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