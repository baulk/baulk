///
#include <bela/phmap.hpp>
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {
struct bucket_status {
  std::wstring latest;
  std::wstring updated;
};

class BucketUpdater {
public:
  BucketUpdater() = default;
  BucketUpdater(const BucketUpdater &) = delete;
  BucketUpdater &operator=(const BucketUpdater &) = delete;
  bool Initialize();
  bool Immobilized();
  bool Update(const baulk::Bucket &bucket);

private:
  bela::flat_hash_map<std::wstring, bucket_status> status;
  std::wstring bucketslock;
};

bool BucketUpdater::Initialize() {
  bucketslock =
      bela::StringCat(baulk::BaulkRoot(), L"\\bucket\\buckets.lock.json");

  return true;
}

bool BucketUpdater::Immobilized() {
  try {

  } catch (const std::exception &e) {
    return false;
  }

  return true;
}

bool BucketUpdater::Update(const baulk::Bucket &bucket) {
  //
  return true;
}
int cmd_update(const argv_t &argv) {
  BucketUpdater updater;
  if (!updater.Initialize()) {
    return 1;
  }
  for (const auto &bucket : baulk::BaulkBuckets()) {
    updater.Update(bucket);
  }
  if (!updater.Immobilized()) {
    return 1;
  }
  return 0;
}
} // namespace baulk::commands