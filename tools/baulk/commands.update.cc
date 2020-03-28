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
  bool updated{false};
};

bool BucketUpdater::Initialize() {
  bucketslock =
      bela::StringCat(baulk::BaulkRoot(), L"\\buckets\\buckets.lock.json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, bucketslock.data(), L"rb"); en != 0) {
    if (en == ENOENT) {
      return true;
    }
    auto ec = bela::make_stdc_error_code(en);
    bela::FPrintF(stderr, L"unable load %s error: %s\n", bucketslock,
                  ec.message);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto j = nlohmann::json::parse(fd);
    for (const auto &a : j) {
      if (!a.is_object()) {
        continue;
      }
      auto name = a["name"].get<std::string_view>();
      auto latest = a["latest"].get<std::string_view>();
      auto time = a["time"].get<std::string_view>();
      status.emplace(bela::ToWide(name),
                     bucket_metadata{bela::ToWide(latest), std::string(time)});
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
      bela::FPrintF(stderr, L"unable update %s error: %s\n", bucketslock,
                    ec.message);
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
  updated = true;
  return true;
}

struct PackageU {
  void Add(std::wstring_view name, std::wstring_view version) {
    auto nw = bela::StringWidth(name);
    namewidth.emplace_back(nw);
    auto vw = bela::StringWidth(version);
    versionwidth.emplace_back(vw);
    namemax = (std::max)(nw, namemax);
    versionmax = (std::max)(vw, versionmax);
    names.emplace_back(name);
    versions.emplace_back(version);
  }
  std::wstring Pretty() {
    std::wstring space;
    space.resize((std::max)(versionmax, namemax) + 2, L' ');
    std::wstring s;
    for (size_t i = 0; i < names.size(); i++) {
      auto frozen = baulk::BaulkIsFrozenPkg(names[i]);
      bela::StrAppend(&s, names[i], space.substr(0, namemax + 2 - namewidth[i]),
                      L"\x1b[32m", versions[i], L"\x1b[0m",
                      space.substr(0, versionmax + 2 - versionwidth[i]),
                      frozen ? L"\x1b[33mfrozen\x1b[0m\n" : L"\n");
    }
    return s;
  }
  std::vector<std::wstring> names;
  std::vector<std::wstring> versions;
  std::vector<size_t> namewidth;
  std::vector<size_t> versionwidth;
  size_t namemax{0};
  size_t versionmax{0};
};

bool PackageScanUpdatable() {
  PackageU pkgu;
  baulk::fs::Finder finder;
  bela::error_code ec;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  if (!finder.First(locksdir, L"*.json", ec)) {
    return true;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    auto name = finder.Name();
    if (!bela::EndsWithIgnoreCase(name, L".json")) {
      continue;
    }
    baulk::Package pkg;
    name.remove_suffix(5);
    if (baulk::bucket::PackageIsUpdatable(name, pkg)) {
      pkgu.Add(pkg.name, pkg.version);
    }
  } while (finder.Next());
  if (pkgu.names.empty()) {
    return true;
  }
  bela::FPrintF(stderr, L"\x1b[33m%d packages can be updated.\x1b[0m\n%s\n",
                pkgu.names.size(), pkgu.Pretty());
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
  PackageScanUpdatable();
  return 0;
}
} // namespace baulk::commands
