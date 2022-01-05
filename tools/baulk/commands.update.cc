///
#include <bela/phmap.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <bela/datetime.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include <baulk/net.hpp>
#include <baulk/fs.hpp>
#include <baulk/json_utils.hpp>
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
      bela::flat_hash_map<std::wstring, bucket_metadata, net::net_internal::StringCaseInsensitiveHash,
                          net::net_internal::StringCaseInsensitiveEq>;
  BucketUpdater() = default;
  BucketUpdater(const BucketUpdater &) = delete;
  BucketUpdater &operator=(const BucketUpdater &) = delete;
  bool Initialize();
  bool Immobilized();
  bool Update(const baulk::Bucket &bucket);

private:
  bucket_status_t status;
  std::wstring lockFile;
  bool updated{false};
};

bool BucketUpdater::Initialize() {
  lockFile = bela::StringCat(vfs::AppBuckets(), L"\\buckets.lock.json");
  bela::error_code ec;
  auto jo = json::parse_file(lockFile, ec);
  if (!jo) {
    return ec.code == ENOENT;
  }
  try {
    for (const auto &a : jo->obj) {
      if (!a.is_object()) {
        continue;
      }
      auto name = a["name"].get<std::string_view>();
      auto latest = a["latest"].get<std::string_view>();
      auto time = a["time"].get<std::string_view>();
      status.emplace(bela::encode_into<char, wchar_t>(name),
                     bucket_metadata{bela::encode_into<char, wchar_t>(latest), std::string(time)});
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk update: decode metadata. error: %s\n", e.what());
    return false;
  }
  return true;
}

bool BucketUpdater::Immobilized() {
  if (!updated) {
    return true;
  }
  try {
    nlohmann::json j;
    for (const auto &b : status) {
      nlohmann::json o;
      o["name"] = bela::encode_into<wchar_t, char>(b.first);
      o["latest"] = bela::encode_into<wchar_t, char>(b.second.latest);
      o["time"] = b.second.updated;
      j.push_back(std::move(o));
    }
    auto meta = j.dump(4);
    bela::error_code ec;
    if (!bela::io::WriteTextAtomic(meta, lockFile, ec)) {
      bela::FPrintF(stderr, L"baulk unable: update %s error: %s\n", lockFile, ec);
      return false;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk update: unable encode metadata. error: %s\n", e.what());
    return false;
  }

  return true;
}

bool BucketUpdater::Update(const baulk::Bucket &bucket) {
  bela::error_code ec;
  auto latest = baulk::BucketNewest(bucket, ec);
  if (!latest) {
    bela::FPrintF(stderr, L"baulk update \x1b[34m%s\x1b[0m error: \x1b[31m%s\x1b[0m\n", bucket.name, ec);
    return false;
  }
  auto it = status.find(bucket.name);
  if (it != status.end() && bela::EqualsIgnoreCase(it->second.latest, *latest)) {
    baulk::DbgPrint(L"bucket: %s is up to date. id: %s", bucket.name, *latest);
    return true;
  }
  baulk::DbgPrint(L"bucket: %s latest id: %s", bucket.name, *latest);
  if (!baulk::BucketUpdate(bucket, *latest, ec)) {
    bela::FPrintF(stderr, L"bucke download \x1b[34m%s\x1b[0m error: \x1b[31m%s\x1b[0m\n", bucket.name, ec);
    return false;
  }
  bela::FPrintF(stderr, L"\x1b[32m'%s' is up to date: %s\x1b[0m\n", bucket.name, *latest);
  status[bucket.name] = bucket_metadata{*latest, bela::FormatTime<char>(bela::Now())};
  updated = true;
  return true;
}

bool PackageScanUpdatable() {
  bela::fs::Finder finder;
  bela::error_code ec;
  size_t upgradable = 0;
  if (finder.First(vfs::AppLocks(), L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgName = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgName, L".json")) {
        continue;
      }
      pkgName.remove_suffix(5);
      auto localMeta = baulk::PackageLocalMeta(pkgName, ec);
      if (!localMeta) {
        continue;
      }
      baulk::Package pkg;
      if (baulk::PackageUpdatableMeta(*localMeta, pkg)) {
        upgradable++;
        bela::FPrintF(stderr,
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                      localMeta->name, localMeta->bucket, localMeta->version, pkg.version, pkg.bucket,
                      IsFrozenedPackage(pkgName) ? L" \x1b[33m(frozen)\x1b[0m" : L"", StringCategory(pkg));
        continue;
      }
    } while (finder.Next());
  }
  bela::FPrintF(stderr, L"\x1b[32m%d packages can be updated.\x1b[0m\n", upgradable);
  return true;
}

void usage_update() {
  bela::FPrintF(stderr, LR"(Usage: baulk update
Update bucket metadata.

)");
}

int UpdateBucket(bool showUpdatable) {
  BucketUpdater updater;
  if (!updater.Initialize()) {
    return 1;
  }
  for (const auto &bucket : baulk::LoadedBuckets()) {
    updater.Update(bucket);
  }
  if (!updater.Immobilized()) {
    return 1;
  }
  if (showUpdatable) {
    PackageScanUpdatable();
  }
  return 0;
}

int cmd_update(const argv_t & /*unused argv*/) {
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk update: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  return UpdateBucket(true);
}
} // namespace baulk::commands
