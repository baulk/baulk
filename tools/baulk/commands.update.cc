///
#include <bela/phmap.hpp>
#include <bela/path.hpp>
#include <jsonex.hpp>
#include <time.hpp>
#include "net.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {
struct bucket_metadata {
  std::wstring latest;
  std::string updated;
};

class BucketUpdater {
public:
  using bucket_status_t =
      bela::flat_hash_map<std::wstring, bucket_metadata,
                          baulk::net::StringCaseInsensitiveHash,
                          baulk::net::StringCaseInsensitiveEq>;
  BucketUpdater() = default;
  BucketUpdater(const BucketUpdater &) = delete;
  BucketUpdater &operator=(const BucketUpdater &) = delete;
  bool Initialize();
  bool Immobilized();
  bool Update(const baulk::Bucket &bucket);

private:
  bucket_status_t status;
  std::wstring bucketslock;
};

bool BucketUpdater::Initialize() {
  bucketslock =
      bela::StringCat(baulk::BaulkRoot(), L"\\buckets\\buckets.lock.json");

  return true;
}

bool BucketUpdater::Immobilized() {
  try {
    nlohmann::json j;
    for (const auto &b : status) {
    }
  } catch (const std::exception &e) {
    return false;
  }

  return true;
}

bool BucketUpdater::Update(const baulk::Bucket &bucket) {
  bela::error_code ec;
  auto latest = baulk::bucket::BucketNewest(bucket.url, ec);
  if (!latest) {
    bela::FPrintF(stderr, L"update %s error: %s\n", bucket.name, ec.message);
    return false;
  }
  auto it = status.find(bucket.name);
  if (it != status.end() &&
      bela::EqualsIgnoreCase(it->second.latest, *latest)) {
    baulk::DbgPrint(L"bucket: %s is up to date. id: %s", bucket.name, *latest);
    return true;
  }
  baulk::DbgPrint(L"bucket: %s latest id: %s", bucket.name, *latest);
  if (!baulk::bucket::BucketUpdate(bucket.url, bucket.name, *latest, ec)) {
    bela::FPrintF(stderr, L"bucke: %s download data error: %s\n", bucket.name,
                  ec.message);
    return false;
  }
  bela::FPrintF(stderr, L"\x1b[32m'%s' is up to date: %s\x1b[0m\n", bucket.name,
                *latest);
  status[bucket.name] = bucket_metadata{*latest, baulk::time::TimeNow()};
  return true;
} // namespace baulk::commands
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
