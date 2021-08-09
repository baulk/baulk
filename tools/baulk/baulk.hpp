///
#ifndef BAULK_HPP
#define BAULK_HPP
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include "compiler.hpp"

namespace baulk {
extern bool IsDebugMode;
extern bool IsForceMode;
extern bool IsForceDelete;
extern bool IsQuietMode;
extern bool IsTraceMode;
extern bool IsInsecureMode;
constexpr size_t UerAgentMaximumLength = 256;
extern wchar_t UserAgent[UerAgentMaximumLength];
// DbgPrint added newline
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, const Args &...args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

template <typename... Args> bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt, const Args &...args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  auto str = bela::StringCat(L"\x1b[32m* ", prefix, L" ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[32m", prefix, L" ", msg, L"\x1b[0m\n"));
}

/// defines
[[maybe_unused]] constexpr std::wstring_view BucketsDirName = L"buckets";
enum BucketObserveMode : int {
  Github = 0, // ZIP
  Git = 1
};

inline const std::wstring_view BucketObserveModeName(BucketObserveMode m) {
  switch (m) {
  case BucketObserveMode::Github:
    break;
  case BucketObserveMode::Git:
    return L"Git";
  default:
    break;
  }
  return L"Github";
}

enum InstallMode : int {
  Portable = 0, // Portable
  User = 1,     // User Installer
  System = 2,   // System Installer
};
InstallMode FindInstallMode();
inline const std::wstring_view InstallModeName(InstallMode m) {
  switch (m) {
  case InstallMode::Portable:
    break;
  case InstallMode::User:
    return L"User Installer";
  case InstallMode::System:
    return L"System Installer";
  default:
    break;
  }
  return L"Portable";
}

struct Bucket {
  Bucket() = default;
  Bucket(std::wstring_view desc, std::wstring_view n, std::wstring_view u, int weights_ = 99,
         BucketObserveMode mode_ = BucketObserveMode::Github) {
    description = desc;
    name = n;
    url = u;
    weights = weights_;
    mode = mode_;
  }
  std::wstring description;
  std::wstring name;
  std::wstring url;
  int weights{99};
  BucketObserveMode mode{BucketObserveMode::Github};
};

using Buckets = std::vector<Bucket>;

// Env functions
bool InitializeBaulkEnv(int argc, wchar_t *const *argv, std::wstring_view profile);
std::wstring_view BaulkProfile();
bool BaulkIsFrozenPkg(std::wstring_view pkg);
std::wstring_view BaulkRoot();
std::wstring_view BaulkLocale();
Buckets &BaulkBuckets();
int BaulkBucketWeights(std::wstring_view bucket);
std::wstring_view BaulkGit();
baulk::compiler::Executor &BaulkExecutor();
bool BaulkInitializeExecutor(bela::error_code &ec);
// package base

struct LinkMeta {
  LinkMeta() = default;
  LinkMeta(std::wstring_view path_, std::wstring_view alias_) : path(path_), alias(alias_) {}
  LinkMeta(std::wstring_view sv) {
    if (auto pos = sv.find('@'); pos != std::wstring_view::npos) {
      path.assign(sv.data(), pos);
      alias.assign(sv.substr(pos + 1));
      return;
    }
    path.assign(sv);
    alias.assign(bela::BaseName(path));
  }
  LinkMeta(const LinkMeta &lm) : path(lm.path), alias(lm.alias) {}
  std::wstring path;
  std::wstring alias;
};

struct BaulkVirtualEnv {
  std::wstring category;
  std::vector<std::wstring> paths;
  std::vector<std::wstring> includes;
  std::vector<std::wstring> libs;
  std::vector<std::wstring> envs;
  std::vector<std::wstring> dependencies;
  std::vector<std::wstring> mkdirs;
  bool empty() const {
    return paths.empty() && includes.empty() && libs.empty() && envs.empty() && dependencies.empty();
  }
};

struct Package {
  std::wstring name;
  std::wstring description;
  std::wstring version;
  std::wstring bucket;
  std::wstring checksum;
  std::wstring extension;
  std::wstring rename; // only exe extension support rename feature
  std::wstring homepage;
  std::wstring notes;
  std::wstring license;
  std::vector<std::wstring> urls;
  std::vector<std::wstring> forceDeletes; // uninstall delete dirs
  std::vector<std::wstring> suggest;
  std::vector<LinkMeta> links;
  std::vector<LinkMeta> launchers;
  BaulkVirtualEnv venv;
  int weights{0}; // Weights derived from bucket
};

class BaulkCloser {
public:
  BaulkCloser(HANDLE hFile_) : FileHandle(hFile_) {}
  BaulkCloser(const BaulkCloser &) = delete;
  BaulkCloser &operator=(const BaulkCloser &) = delete;
  ~BaulkCloser();
  // FileDispositionInfo or FILE_FLAG_DELETE_ON_CLOSE
  static std::optional<BaulkCloser> BaulkMakeLocker(bela::error_code &ec);

private:
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
};

} // namespace baulk

#endif
