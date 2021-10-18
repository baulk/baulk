// initialize baulk environment
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/str_cat_narrow.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include "baulk.hpp"
#include <baulk/fs.hpp>

namespace baulk {
// https://github.com/baulk/bucket/commits/master.atom
constexpr std::wstring_view DefaultBucket = L"https://github.com/baulk/bucket";

class BaulkEnv {
public:
  BaulkEnv(const BaulkEnv &) = delete;
  BaulkEnv &operator=(const BaulkEnv &) = delete;
  bool Initialize(int argc, wchar_t *const *argv, std::wstring_view profile_);
  bool InitializeFallback() {
    buckets.emplace_back(L"Baulk default bucket", L"Baulk", DefaultBucket);
    return true;
  }
  bool InitializeExecutor(bela::error_code &ec) {
    if (!executor.Initialize()) {
      ec = executor.LastErrorCode();
      return false;
    }
    return true;
  }
  static BaulkEnv &Instance() {
    static BaulkEnv baulkEnv;
    return baulkEnv;
  }
  std::wstring_view BaulkRoot() const { return root; }
  std::wstring_view Profile() const { return profile; }
  std::wstring_view Locale() const { return locale; }
  std::wstring_view Git() const { return git; }
  baulk::compiler::Executor &BaulkExecutor() { return executor; }
  Buckets &BaulkBuckets() { return buckets; }
  int BaulkBucketWeights(std::wstring_view bucket) {
    for (const auto &bk : buckets) {
      if (bela::EqualsIgnoreCase(bk.name, bucket)) {
        return bk.weights;
      }
    }
    return 0;
  }
  bool IsFrozen(std::wstring_view pkg) const {
    for (const auto &p : freezepkgs) {
      if (pkg == p) {
        return true;
      }
    }
    return false;
  }

private:
  BaulkEnv() = default;
  std::wstring root;
  std::wstring git;
  std::wstring profile;
  std::wstring locale; // mirrors
  Buckets buckets;
  std::vector<std::wstring> freezepkgs;
  baulk::compiler::Executor executor;
};

std::optional<std::wstring> FindBaulkInstallPath(bool systemTarget, bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW((systemTarget ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER), LR"(SOFTWARE\Baulk)", &hkey) !=
      ERROR_SUCCESS) {
    return std::nullopt;
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) !=
      ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"InstallPath not REG_SZ: ", type);
    return std::nullopt;
  }
  return std::make_optional<std::wstring>(buffer);
}

inline std::wstring BaulkRootPath() {
  bela::error_code ec;
  if (auto exedir = bela::ExecutableParent(ec); exedir) {
    return std::wstring(bela::DirName(*exedir));
  }
  bela::FPrintF(stderr, L"unable find executable path: %s\n", ec.message);
  return L".";
}

InstallMode FindInstallMode() {
  std::filesystem::path baulkRoot(BaulkRootPath());
  bela::error_code ec;
  if (auto installPath = FindBaulkInstallPath(true, ec); installPath) {
    baulk::DbgPrint(L"Find Baulk System Installer: %s", *installPath);
    if (baulkRoot.compare(*installPath) == 0) {
      return InstallMode::System;
    }
  }
  if (auto installPath = FindBaulkInstallPath(false, ec); installPath) {
    baulk::DbgPrint(L"Find Baulk User Installer: %s", *installPath);
    if (baulkRoot.compare(*installPath) == 0) {
      return InstallMode::User;
    }
  }
  return InstallMode::Portable;
}

bool InitializeGitPath(std::wstring &git) {
  if (bela::env::LookPath(L"git.exe", git, true)) {
    baulk::DbgPrint(L"Found git in path '%s'", git);
    return true;
  }
  bela::error_code ec;
  auto installPath = baulk::regutils::GitForWindowsInstallPath(ec);
  if (!installPath) {
    return false;
  }
  git = bela::StringCat(*installPath, L"\\cmd\\git.exe");
  if (bela::PathExists(git)) {
    baulk::DbgPrint(L"Found installed git '%s'", git);
    return true;
  }
  return false;
}

std::wstring ProfileResolve(std::wstring_view profile, std::wstring_view root) {
  if (!profile.empty()) {
    // UNIX style env
    if (profile.find(L'$') != std::wstring_view::npos) {
      return bela::env::PathExpand(profile);
    }
    return bela::WindowsExpandEnv(profile);
  }
  auto p = bela::StringCat(root, L"\\config\\baulk.json");
  if (bela::PathExists(p)) {
    return p;
  }
  return bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\baulk\\baulk.json");
}

inline std::wstring BaulkLocaleName() {
  std::wstring s;
  s.resize(64);
  if (auto n = GetUserDefaultLocaleName(s.data(), 64); n != 0 && n < 64) {
    s.resize(n);
    return s;
  }
  return L"";
}

bool BaulkEnv::Initialize(int argc, wchar_t *const *argv, std::wstring_view profile_) {
  root = BaulkRootPath();
  baulk::DbgPrint(L"Baulk root '%s'", root);
  locale = BaulkLocaleName();
  baulk::DbgPrint(L"Baulk locale name %s\n", locale);
  profile = ProfileResolve(profile_, root);
  baulk::DbgPrint(L"Expand profile to '%s'", profile);
  if (!InitializeGitPath(git)) {
    git.clear();
  }
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, profile.data(), L"rb"); eo != 0) {
    auto ec = bela::make_stdc_error_code(eo);
    DbgPrint(L"unable open bucket: %s %s\n", profile, ec.message);
    return InitializeFallback();
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto json = nlohmann::json::parse(fd, nullptr, true, true);
    // overload locale
    if (auto it = json.find("locale"); it != json.end() && it.value().is_string()) {
      locale = bela::encode_into<char, wchar_t>(it.value().get<std::string_view>());
    }
    const auto &bks = json["bucket"];
    for (auto &b : bks) {
      auto desc = bela::encode_into<char, wchar_t>(b["description"].get<std::string_view>());
      auto name = bela::encode_into<char, wchar_t>(b["name"].get<std::string_view>());
      auto url = bela::encode_into<char, wchar_t>(b["url"].get<std::string_view>());
      int weights = 100;
      if (auto it = b.find("weights"); it != b.end()) {
        weights = it.value().get<int>();
      }
      BucketObserveMode mode{BucketObserveMode::Github};
      if (auto it = b.find("mode"); it != b.end()) {
        if (auto mode_ = it.value().get<int>(); mode_ == 1) {
          mode = BucketObserveMode::Git;
        }
      }
      DbgPrint(L"Add bucket: %s '%s@%s' %s", url, name, desc, BucketObserveModeName(mode));
      buckets.emplace_back(std::move(desc), std::move(name), std::move(url), weights, mode);
    }
    if (auto it = json.find("freeze"); it != json.end()) {
      for (const auto &freeze : it.value()) {
        freezepkgs.emplace_back(bela::encode_into<char, wchar_t>(freeze.get<std::string_view>()));
        DbgPrint(L"Freeze package %s", freezepkgs.back());
      }
    }
  } catch (const std::exception &) {
    return InitializeFallback();
  }
  return true;
}

bool InitializeBaulkEnv(int argc, wchar_t *const *argv, std::wstring_view profile) {
  return BaulkEnv::Instance().Initialize(argc, argv, profile);
}
bool BaulkInitializeExecutor(bela::error_code &ec) { return BaulkEnv::Instance().InitializeExecutor(ec); }

std::wstring_view BaulkRoot() { return BaulkEnv::Instance().BaulkRoot(); }
Buckets &BaulkBuckets() { return BaulkEnv::Instance().BaulkBuckets(); }
std::wstring_view BaulkGit() { return BaulkEnv::Instance().Git(); }
std::wstring_view BaulkProfile() { return BaulkEnv::Instance().Profile(); }
std::wstring_view BaulkLocale() { return BaulkEnv::Instance().Locale(); }
baulk::compiler::Executor &BaulkExecutor() { return BaulkEnv::Instance().BaulkExecutor(); }
int BaulkBucketWeights(std::wstring_view bucket) { return BaulkEnv::Instance().BaulkBucketWeights(bucket); }
bool BaulkIsFrozenPkg(std::wstring_view pkg) { return BaulkEnv::Instance().IsFrozen(pkg); }

inline bool BaulkIsRunning(DWORD pid) {
  if (HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid); hProcess != nullptr) {
    CloseHandle(hProcess);
    return true;
  }
  return false;
}

BaulkCloser::~BaulkCloser() {
  if (FileHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(FileHandle);
    auto file = bela::StringCat(BaulkEnv::Instance().BaulkRoot(), L"\\bin\\pkgs\\baulk.pid");
    DeleteFileW(file.data());
  }
}

std::optional<BaulkCloser> BaulkCloser::BaulkMakeLocker(bela::error_code &ec) {
  auto pkgsdir = bela::StringCat(BaulkEnv::Instance().BaulkRoot(), L"\\bin\\pkgs");
  if (!baulk::fs::MakeDir(pkgsdir, ec)) {
    return std::nullopt;
  }
  auto file = bela::StringCat(pkgsdir, L"\\baulk.pid");
  if (auto line = bela::io::ReadLine(file, ec); line) {
    DWORD pid = 0;
    if (bela::SimpleAtoi(*line, &pid) && BaulkIsRunning(pid)) {
      ec = bela::make_error_code(bela::ErrGeneral, L"baulk is running. pid= ", pid);
      return std::nullopt;
    }
  }
  auto FileHandle = ::CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  bela::narrow::AlphaNum an(GetCurrentProcessId());
  DWORD written = 0;
  if (WriteFile(FileHandle, an.data(), static_cast<DWORD>(an.size()), &written, nullptr) != TRUE) {
    ec = bela::make_system_error_code();
    CloseHandle(FileHandle);
    DeleteFileW(file.data());
    return std::nullopt;
  }
  return std::make_optional<BaulkCloser>(FileHandle);
}

} // namespace baulk