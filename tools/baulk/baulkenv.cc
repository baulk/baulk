// initialize baulk environment
#include "baulk.hpp"
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>

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

private:
  BaulkEnv() = default;
  std::wstring root;
  std::wstring git;
  std::wstring profile;
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
  bela::error_code ec;
  if (auto exedir = bela::ExecutablePath(ec); exedir) {
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
      DbgPrint(L"Add bucket: %s '%s@%s'", url, name, desc);
      buckets.emplace_back(std::move(desc), std::move(name), std::move(url));
    }
    if (auto it = json.find("freeze"); it != json.end()) {
      for (const auto &freeze : it.value()) {
        freezepkgs.emplace_back(
            bela::ToWide(freeze.get_ref<const std::string &>()));
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

std::wstring_view BaulkProfile() {
  //
  return BaulkEnv::Instance().Profile();
}

baulk::compiler::Executor &BaulkExecutor() {
  return BaulkEnv::Instance().BaulkExecutor();
}

bool BaulkInitializeExecutor(bela::error_code &ec) {
  return BaulkEnv::Instance().InitializeExecutor(ec);
}

} // namespace baulk