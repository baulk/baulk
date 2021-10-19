// baulk context
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include "baulk.hpp"

namespace baulk {
namespace baulk_internal {
inline std::wstring path_expand(std::wstring_view p) {
  if (p.find(L'$') != std::wstring_view::npos) {
    return bela::env::PathExpand(p);
  }
  return bela::WindowsExpandEnv(p);
}

inline std::wstring default_locale_name() {
  std::wstring s;
  s.resize(64);
  if (auto n = GetUserDefaultLocaleName(s.data(), 64); n != 0 && n < 64) {
    s.resize(n);
    return s;
  }
  return L"";
}
// https://github.com/baulk/bucket/commits/master.atom
constexpr std::wstring_view DefaultBucket = L"https://github.com/baulk/bucket";
} // namespace baulk_internal

class Context {
public:
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  static Context &Instance() {
    static Context inst;
    return inst;
  }
  bool Initialize(std::wstring_view profile_, bela::error_code &ec);
  bool InitializeExecutor(bela::error_code &ec);
  bool IsFrozened(std::wstring_view pkg) const { return std::find(fpkgs.begin(), fpkgs.end(), pkg) != fpkgs.end(); }
  std::wstring_view LocaleName() const { return localeName; }

private:
  bool initializeInternal(const std::wstring &profile_, bela::error_code &ec);
  Context() = default;
  std::wstring profile;
  std::wstring localeName; // mirrors
  Buckets buckets;
  std::vector<std::wstring> fpkgs;
  baulk::compiler::Executor executor;
};

bool Context::initializeInternal(const std::wstring &profile_, bela::error_code &ec) {
  DbgPrint(L"Baulk use profile '%s'", profile_);
  auto jo = baulk::json::parse_file(profile_, ec);
  if (!jo) {
    buckets.emplace_back(L"Baulk default bucket", L"Baulk", baulk_internal::DefaultBucket);
    bela::FPrintF(stderr, L"Baulk parse profile %s error: \x1b[31m%s\x1b[0m, ignore profile.\n", profile_, ec.message);
    return true;
  }
  auto jv = jo->view();
  localeName = jv.fetch("locale", localeName);
  auto svs = jv.subviews("buckets");
  for (auto sv : svs) {
    buckets.emplace_back(sv.fetch("description"), sv.fetch("name"), sv.fetch("url"),
                         sv.fetch_as_integer("weights", 100), sv.fetch_as_integer("mode", 0));
    auto bk = buckets.back();
    DbgPrint(L"Add bucket: %s '%s@%s' %s", bk.url, bk.name, bk.description, BucketObserveModeName(bk.mode));
  }
  if (jv.fetch_strings_checked("freeze", fpkgs) && !fpkgs.empty() && IsDebugMode) {
    for (const auto &p : fpkgs) {
      DbgPrint(L"Freeze package %s", p);
    }
  }
  return true;
}

bool Context::Initialize(std::wstring_view profile_, bela::error_code &ec) {
  if (!baulk::vfs::InitializePathFs(ec)) {
    return false;
  }
  DbgPrint(L"Baulk root '%s'", baulk::vfs::AppBasePath());
  localeName = baulk_internal::default_locale_name();
  baulk::DbgPrint(L"Baulk locale name %s", localeName);
  if (profile_.empty()) {
    return initializeInternal(baulk::vfs::AppDefaultProfile(), ec);
  }
  return initializeInternal(baulk_internal::path_expand(profile_), ec);
}

bool Context::InitializeExecutor(bela::error_code &ec) { return executor.Initialize(ec); }

// global functions
bool InitializeContext(std::wstring_view profile_, bela::error_code &ec) {
  return Context::Instance().Initialize(profile_, ec);
}
bool InitializeExecutor(bela::error_code &ec) { return Context::Instance().InitializeExecutor(ec); }
std::wstring_view LocaleName() { return Context::Instance().LocaleName(); }

} // namespace baulk