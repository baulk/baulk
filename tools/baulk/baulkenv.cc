// initialize baulk environment
#include "baulk.hpp"
#include <bela/env.hpp>
#include "jsonex.hpp"

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

private:
  BaulkEnv() = default;
  std::wstring root;
  std::wstring bucketUrl{DefaultBucket};
};

bool BaulkEnv::Initialize(int argc, wchar_t *const *argv,
                          std::wstring_view profile) {
  auto profile_ = bela::ExpandEnv(profile.empty() ? DefaultProfile : profile);
  baulk::DbgPrint(L"Expand profile to '%s'\n", profile_);
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

} // namespace baulk