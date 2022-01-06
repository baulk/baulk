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

/// defines
[[maybe_unused]] constexpr std::wstring_view BucketsDirName = L"buckets";
enum BucketObserveMode {
  Github = 0, // ZIP
  Git = 1
};

inline const std::wstring_view BucketObserveModeName(BucketObserveMode m) {
  switch (m) {
  case BucketObserveMode::Github:
    break;
  case BucketObserveMode::Git:
    return L"git";
  default:
    break;
  }
  return L"github-zip";
}

enum BucketVariant {
  Native = 0, //
  Scoop = 1,
};

inline const std::wstring_view BucketVariantName(BucketVariant v) {
  switch (v) {
  case BucketVariant::Native:
    break;
  case BucketVariant::Scoop:
    return L"scoop";
  default:
    break;
  }
  return L"native";
}

struct Bucket {
  Bucket() = default;
  Bucket(std::wstring_view desc, std::wstring_view n, std::wstring_view u, int weights_ = 99,
         BucketObserveMode mode_ = BucketObserveMode::Github, BucketVariant variant_ = BucketVariant::Native) {
    description = desc;
    name = n;
    url = u;
    weights = weights_;
    mode = mode_;
    variant = variant_;
  }
  std::wstring description;
  std::wstring name;
  std::wstring url;
  int weights{99};
  BucketObserveMode mode{BucketObserveMode::Github};
  BucketVariant variant{BucketVariant::Native};
};
using Buckets = std::vector<Bucket>;

// Initialize context
bool InitializeContext(std::wstring_view profile, bela::error_code &ec);
bool InitializeExecutor(bela::error_code &ec);
std::wstring_view Profile();
std::wstring_view LocaleName();
Buckets &LoadedBuckets();
compiler::Executor &LinkExecutor();
bool IsFrozenedPackage(std::wstring_view pkgName);
int BucketWeights(std::wstring_view bucket);

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

struct PackageEnv {
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
  std::wstring extension;
  std::wstring rename; // only exe extension support rename feature
  std::wstring homepage;
  std::wstring notes;
  std::wstring license;
  std::wstring hashValue;
  std::vector<std::wstring> urls;
  std::vector<std::wstring> forceDeletes; // uninstall delete dirs
  std::vector<std::wstring> suggest;
  std::vector<LinkMeta> links;
  std::vector<LinkMeta> launchers;
  PackageEnv venv;
  int weights{0}; // Weights derived from bucket
  BucketVariant variant{BucketVariant::Native};
};

inline std::wstring StringCategory(baulk::Package &pkg) {
  if (pkg.venv.category.empty()) {
    return L"";
  }
  return bela::StringCat(L" \x1b[36m[", pkg.venv.category, L"]\x1b[0m");
}

} // namespace baulk

#endif
