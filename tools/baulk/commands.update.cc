///
#include <bela/phmap.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <filesystem>
#include <jsonex.hpp>
#include <time.hpp>
#include "net.hpp"
#include "bucket.hpp"
#include "fs.hpp"
#include "commands.hpp"

namespace baulk::commands {
struct bucket_metadata {
  std::wstring latest;
  std::string updated;
};

class BucketUpdater {
public:
  using bucket_status_t = bela::flat_hash_map<std::wstring, bucket_metadata, baulk::net::StringCaseInsensitiveHash,
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
  bool updated{false};
};

bool BucketUpdater::Initialize() {
  bucketslock = bela::StringCat(baulk::BaulkRoot(), L"\\buckets\\buckets.lock.json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, bucketslock.data(), L"rb"); en != 0) {
    if (en == ENOENT) {
      return true;
    }
    auto ec = bela::make_stdc_error_code(en);
    bela::FPrintF(stderr, L"unable load %s error: %s\n", bucketslock, ec.message);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto j = nlohmann::json::parse(fd, nullptr, true, true);
    for (const auto &a : j) {
      if (!a.is_object()) {
        continue;
      }
      auto name = a["name"].get<std::string_view>();
      auto latest = a["latest"].get<std::string_view>();
      auto time = a["time"].get<std::string_view>();
      status.emplace(bela::ToWide(name), bucket_metadata{bela::ToWide(latest), std::string(time)});
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable decode metadata. error: %s\n", e.what());
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
      o["name"] = bela::ToNarrow(b.first);
      o["latest"] = bela::ToNarrow(b.second.latest);
      o["time"] = b.second.updated;
      j.push_back(std::move(o));
    }
    auto meta = j.dump(4);
    bela::error_code ec;
    if (!bela::io::WriteTextAtomic(meta, bucketslock, ec)) {
      bela::FPrintF(stderr, L"unable update %s error: %s\n", bucketslock, ec.message);
      return false;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable encode metadata. error: %s\n", e.what());
    return false;
  }

  return true;
}

bool BucketUpdater::Update(const baulk::Bucket &bucket) {
  bela::error_code ec;
  auto latest = baulk::bucket::BucketNewest(bucket, ec);
  if (!latest) {
    bela::FPrintF(stderr, L"baulk update \x1b[34m%s\x1b[0m error: \x1b[31m%s\x1b[0m\n", bucket.name, ec.message);
    return false;
  }
  auto it = status.find(bucket.name);
  if (it != status.end() && bela::EqualsIgnoreCase(it->second.latest, *latest)) {
    baulk::DbgPrint(L"bucket: %s is up to date. id: %s", bucket.name, *latest);
    return true;
  }
  baulk::DbgPrint(L"bucket: %s latest id: %s", bucket.name, *latest);
  if (!baulk::bucket::BucketUpdate(bucket, *latest, ec)) {
    bela::FPrintF(stderr, L"bucke download \x1b[34m%s\x1b[0m error: \x1b[31m%s\x1b[0m\n", bucket.name, ec.message);
    return false;
  }
  bela::FPrintF(stderr, L"\x1b[32m'%s' is up to date: %s\x1b[0m\n", bucket.name, *latest);
  status[bucket.name] = bucket_metadata{*latest, baulk::time::TimeNow()};
  updated = true;
  return true;
}

inline std::wstring StringCategory(baulk::Package &pkg) {
  if (pkg.venv.category.empty()) {
    return L"";
  }
  return bela::StringCat(L" \x1b[36m[", pkg.venv.category, L"]\x1b[0m");
}

bool PackageScanUpdatable() {
  bela::fs::Finder finder;
  bela::error_code ec;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  size_t upgradable = 0;
  if (finder.First(locksdir, L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgname = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgname, L".json")) {
        continue;
      }
      pkgname.remove_suffix(5);
      auto opkg = baulk::bucket::PackageLocalMeta(pkgname, ec);
      if (!opkg) {
        continue;
      }
      baulk::Package pkg;
      if (baulk::bucket::PackageUpdatableMeta(*opkg, pkg)) {
        upgradable++;
        bela::FPrintF(stderr,
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                      opkg->name, opkg->bucket, opkg->version, pkg.version, pkg.bucket,
                      baulk::BaulkIsFrozenPkg(pkgname) ? L" \x1b[33m(frozen)\x1b[0m" : L"", StringCategory(pkg));
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

int cmd_update(const argv_t &argv) {
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk update: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
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
  PackageScanUpdatable();
  return 0;
}
} // namespace baulk::commands
