// initialize baulk environment
#include "baulk.hpp"
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <filesystem>
#include "jsonex.hpp"
#include "regutils.hpp"

namespace baulk {
// https://github.com/baulk/bucket/commits/master.atom
constexpr std::wstring_view DefaultBucket = L"https://github.com/baulk/bucket";
class BaulkEnv {
public:
  BaulkEnv(const BaulkEnv &) = delete;
  BaulkEnv &operator=(const BaulkEnv &) = delete;
  bool Initialize(int argc, wchar_t *const *argv, std::wstring_view profile);

  static BaulkEnv &Instance() {
    static BaulkEnv baulkEnv;
    return baulkEnv;
  }
  std::wstring_view BaulkRoot() const { return root; }
  Buckets &BaulkBuckets() { return buckets; }
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

private:
  BaulkEnv() = default;
  std::wstring root;
  std::wstring git;
  Buckets buckets;
  std::vector<std::wstring> freezepkgs;
  baulk::compiler::Executor executor;
};

bool InitializeGitPath(std::wstring &git) {
  if (bela::ExecutableExistsInPath(L"git.exe", git)) {
    baulk::DbgPrint(L"Found git in path '%s'\n", git);
    return true;
  }
  bela::error_code ec;
  auto installPath = baulk::regutils::GitForWindowsInstallPath(ec);
  if (!installPath) {
    return false;
  }
  git = bela::StringCat(*installPath, L"\\cmd\\git.exe");
  if (bela::PathExists(git)) {
    baulk::DbgPrint(L"Found installed git '%s'\n", git);
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
                          std::wstring_view profile) {
  bela::error_code ec;
  if (auto exedir = bela::ExecutablePath(ec); exedir) {
    root = std::filesystem::path(*exedir).parent_path().wstring();
  } else {
    bela::FPrintF(stderr, L"unable find executable path: %s\n", ec.message);
    root = L".";
  }
  auto profile_ = ProfileResolve(profile, root);
  baulk::DbgPrint(L"Expand profile to '%s'\n", profile_);
  if (!InitializeGitPath(git)) {
    git.clear();
  }

  return true;
}

bool InitializeBaulkEnv(int argc, wchar_t *const *argv,
                        std::wstring_view profile) {
  return BaulkEnv::Instance().Initialize(argc, argv, profile);
}
bool InitializeBaulkBuckets() {
  //
  return true;
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
baulk::compiler::Executor &BaulkExecutor() {
  return BaulkEnv::Instance().BaulkExecutor();
}

bool BaulkInitializeExecutor(bela::error_code &ec) {
  return BaulkEnv::Instance().InitializeExecutor(ec);
}

} // namespace baulk