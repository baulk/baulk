// initialize baulk environment
#include "baulk.hpp"
#include <bela/env.hpp>
#include <bela/path.hpp>
#include "jsonex.hpp"
#include "regutils.hpp"

namespace baulk {
// https://github.com/baulk/bucket/commits/master.atom
constexpr std::wstring_view DefaultBucket = L"https://github.com/baulk/bucket";
constexpr std::wstring_view DefaultProfile = L"%LOCALAPPDATA%/baulk/baulk.json";
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
  std::wstring_view BucketUrl() const { return bucketUrl; }
  std::wstring_view Git() const { return git; }

private:
  BaulkEnv() = default;
  std::wstring root;
  std::wstring bucketUrl{DefaultBucket};
  std::wstring git;
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

bool BaulkEnv::Initialize(int argc, wchar_t *const *argv,
                          std::wstring_view profile) {
  auto profile_ = bela::ExpandEnv(profile.empty() ? DefaultProfile : profile);
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

std::wstring_view BaulkRoot() {
  //
  return BaulkEnv::Instance().BaulkRoot();
}
//
std::wstring_view BaulkBucketUrl() {
  // bucket url
  return BaulkEnv::Instance().BucketUrl();
}

std::wstring_view BaulkGit() {
  //
  return BaulkEnv::Instance().Git();
}

} // namespace baulk