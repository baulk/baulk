// initialize baulk environment
#include "baulk.hpp"
#include <bela/env.hpp>

namespace baulk {
// https://github.com/baulk/bucket/commits/master.atom
constexpr std::wstring_view DefaultBucket = L"https://github.com/baulk/bucket";
constexpr std::wstring_view DefaultProfile = L"${HOME}/.baulk.json";
class BaulkEnv {
public:
  BaulkEnv(const BaulkEnv &) = delete;
  BaulkEnv &operator=(const BaulkEnv &) = delete;
  bool Initialize(int argc, wchar_t *const *argv, std::wstring_view profile);

  static BaulkEnv &Instance() {
    static BaulkEnv baulkEnv;
    return baulkEnv;
  }
  std::wstring_view BucketUrl() const { return bucketUrl; }
  std::wstring ExpandEnv(std::wstring_view raw) const {
    std::wstring e;
    dev.ExpandEnv(raw, e);
    return e;
  }

private:
  BaulkEnv() = default;
  std::wstring bucketUrl{DefaultBucket};
  bela::env::Derivator dev;
};

bool BaulkEnv::Initialize(int argc, wchar_t *const *argv,
                          std::wstring_view profile) {
  dev.AddBashCompatible(argc, argv);
  if (profile.empty()) {
    profile = DefaultProfile;
  }
  std::wstring profile_;
  dev.ExpandEnv(profile, profile_);
  return true;
}

bool InitializeBaulkEnv(int argc, wchar_t *const *argv,
                        std::wstring_view profile) {
  return BaulkEnv::Instance().Initialize(argc, argv, profile);
}

std::wstring BaulkExpandEnv(std::wstring_view raw) {
  return BaulkEnv::Instance().ExpandEnv(raw);
}
//
std::wstring_view BaulkBucketUrl() {
  // bucket url
  return BaulkEnv::Instance().BucketUrl();
}

} // namespace baulk