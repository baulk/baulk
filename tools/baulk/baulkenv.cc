// initialize baulk environment
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/narrow/strcat.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include "baulk.hpp"
#include "fs.hpp"

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
  std::wstring_view BaulkRoot() const { return root; }
  Buckets &BaulkBuckets() { return buckets; }
  int BaulkBucketWeights(std::wstring_view bucket) {
    for (const auto &bk : buckets) {
      if (bela::EqualsIgnoreCase(bk.name, bucket)) {
        return bk.weights;
      }
    }
    return 0;
  }
  std::wstring_view Git() const { return git; }
  baulk::compiler::Executor &BaulkExecutor() { return executor; }
  bool InitializeExecutor(bela::error_code &ec) {
    return executor.Initialize(ec);
  }
  bool IsFrozen(std::wstring_view pkg) const {
    for (const auto &p : freezepkgs) {
      if (pkg == p) {
        return true;
      }
    }
    return false;
  }
  //
  static BaulkEnv &Instance() {
    static BaulkEnv baulkEnv;
    return baulkEnv;
  }
  std::wstring_view Profile() const { return profile; }
  std::wstring_view Locale() const { return locale; }

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

bool InitializeGitPath(std::wstring &git) {
  if (bela::ExecutableExistsInPath(L"git.exe", git)) {
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
    return bela::ExpandEnv(profile);
  }
  auto p = bela::StringCat(root, L"\\config\\baulk.json");
  if (bela::PathExists(p)) {
    return p;
  }
  return bela::ExpandEnv(L"%LOCALAPPDATA%\\baulk\\baulk.json");
}

bool BaulkEnv::Initialize(int argc, wchar_t *const *argv,
                          std::wstring_view profile_) {
  //
  locale.resize(64);
  if (auto n = GetUserDefaultLocaleName(locale.data(), 64); n != 0 && n < 64) {
    locale.resize(n);
  } else {
    locale.clear();
  }
  baulk::DbgPrint(L"Baulk locale name %s\n", locale);
  bela::error_code ec;
  if (auto exedir = bela::ExecutableParent(ec); exedir) {
    root.assign(std::move(*exedir));
    bela::PathStripName(root);
  } else {
    bela::FPrintF(stderr, L"unable find executable path: %s\n", ec.message);
    root = L".";
  }
  baulk::DbgPrint(L"Expand root '%s'\n", root);
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
    auto json = nlohmann::json::parse(fd);
    const auto &bks = json["bucket"];
    for (auto &b : bks) {
      auto desc = bela::ToWide(b["description"].get<std::string_view>());
      auto name = bela::ToWide(b["name"].get<std::string_view>());
      auto url = bela::ToWide(b["url"].get<std::string_view>());
      int weights = 100;
      if (auto it = b.find("weights"); it != b.end()) {
        weights = it.value().get<int>();
      }
      DbgPrint(L"Add bucket: %s '%s@%s'", url, name, desc);
      buckets.emplace_back(std::move(desc), std::move(name), std::move(url),
                           weights);
    }
    if (auto it = json.find("freeze"); it != json.end()) {
      for (const auto &freeze : it.value()) {
        freezepkgs.emplace_back(bela::ToWide(freeze.get<std::string_view>()));
        DbgPrint(L"Freeze package %s", freezepkgs.back());
      }
    }
  } catch (const std::exception &) {
    return InitializeFallback();
  }
  return true;
}

bool InitializeBaulkEnv(int argc, wchar_t *const *argv,
                        std::wstring_view profile) {
  return BaulkEnv::Instance().Initialize(argc, argv, profile);
}

bool BaulkIsFrozenPkg(std::wstring_view pkg) {
  //
  return BaulkEnv::Instance().IsFrozen(pkg);
}

std::wstring_view BaulkRoot() {
  //
  return BaulkEnv::Instance().BaulkRoot();
}
Buckets &BaulkBuckets() {
  //
  return BaulkEnv::Instance().BaulkBuckets();
}

std::wstring_view BaulkGit() {
  //
  return BaulkEnv::Instance().Git();
}

std::wstring_view BaulkProfile() {
  //
  return BaulkEnv::Instance().Profile();
}
std::wstring_view BaulkLocale() {
  //
  return BaulkEnv::Instance().Locale();
}

baulk::compiler::Executor &BaulkExecutor() {
  return BaulkEnv::Instance().BaulkExecutor();
}

int BaulkBucketWeights(std::wstring_view bucket) {
  return BaulkEnv::Instance().BaulkBucketWeights(bucket);
}

bool BaulkInitializeExecutor(bela::error_code &ec) {
  return BaulkEnv::Instance().InitializeExecutor(ec);
}

inline bool BaulkIsRunning(DWORD pid) {
  if (HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
      hProcess != nullptr) {
    CloseHandle(hProcess);
    return true;
  }
  return false;
}

BaulkCloser::~BaulkCloser() {
  if (FileHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(FileHandle);
    auto file = bela::StringCat(BaulkEnv::Instance().BaulkRoot(),
                                L"\\bin\\pkgs\\baulk.pid");
    DeleteFileW(file.data());
  }
}

std::optional<BaulkCloser> BaulkCloser::BaulkMakeLocker(bela::error_code &ec) {
  auto pkgsdir =
      bela::StringCat(BaulkEnv::Instance().BaulkRoot(), L"\\bin\\pkgs");
  if (!baulk::fs::MakeDir(pkgsdir, ec)) {
    return std::nullopt;
  }
  auto file = bela::StringCat(pkgsdir, L"\\baulk.pid");
  if (auto line = bela::io::ReadLine(file, ec); line) {
    DWORD pid = 0;
    if (bela::SimpleAtoi(*line, &pid) && BaulkIsRunning(pid)) {
      ec = bela::make_error_code(1, L"baulk is running. pid= ", pid);
      return std::nullopt;
    }
  }
  auto FileHandle =
      ::CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  bela::narrow::AlphaNum an(GetCurrentProcessId());
  DWORD written = 0;
  if (WriteFile(FileHandle, an.data(), static_cast<DWORD>(an.size()), &written,
                nullptr) != TRUE) {
    ec = bela::make_system_error_code();
    CloseHandle(FileHandle);
    DeleteFileW(file.data());
    return std::nullopt;
  }
  return std::make_optional<BaulkCloser>(FileHandle);
}

} // namespace baulk