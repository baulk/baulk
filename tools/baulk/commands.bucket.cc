// bucket
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <json.hpp>
#include "baulk.hpp"
#include "baulkargv.hpp"
#include "commands.hpp"
#include "hash.hpp"
#include "fs.hpp"

namespace baulk::commands {
// Bucket Modifier
class BucketModifier {
public:
  BucketModifier() = default;
  BucketModifier(const BucketModifier &) = delete;
  BucketModifier &operator=(const BucketModifier &) = delete;
  bool Initialize();
  bool Apply();
  bool GetBucket(std::wstring_view bucketName, baulk::Bucket &bucket);
  int List(std::wstring_view bucketName = L"");
  int Add(const baulk::Bucket &bucket, bool replace);
  int Del(const baulk::Bucket &bucket, bool forcemode);
  const bela::error_code &ErrorCode() const { return ec; }

private:
  bela::error_code ec;
  nlohmann::json meta;
};

bool BucketModifier::Initialize() {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, baulk::BaulkProfile().data(), L"rb"); eo != 0) {
    ec = bela::make_stdc_error_code(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    meta = nlohmann::json::parse(fd, nullptr, true, true);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::ToWide(e.what()));
    return false;
  }
  return true;
}

bool BucketModifier::Apply() {
  auto jsontext = meta.dump(4);
  return bela::io::WriteTextAtomic(jsontext, baulk::BaulkProfile(), ec);
}

bool BucketModifier::GetBucket(std::wstring_view bucketName, baulk::Bucket &bucket) {
  try {
    const auto &bks = meta["bucket"];
    for (auto &b : bks) {
      auto name = bela::ToWide(b["name"].get<std::string_view>());
      if (!bela::EqualsIgnoreCase(bucketName, name)) {
        continue;
      }
      bucket.name = name;
      bucket.description = bela::ToWide(b["description"].get<std::string_view>());
      bucket.url = bela::ToWide(b["url"].get<std::string_view>());
      int weights = 100;
      if (auto it = b.find("weights"); it != b.end()) {
        weights = it.value().get<int>();
      }
      bucket.weights = weights;
      bucket.mode = BucketObserveMode::Github;
      if (auto it = b.find("mode"); it != b.end()) {
        if (auto mode_ = it.value().get<int>(); mode_ == 1) {
          bucket.mode = BucketObserveMode::Git;
        }
      }
      return true;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk bucket list: %s\n", e.what());
    return false;
  }
  return false;
}

int BucketModifier::List(std::wstring_view bucketName) {
  if (!bucketName.empty()) {
    baulk::Bucket bk;
    if (!GetBucket(bucketName, bk)) {
      bela::FPrintF(stderr, L"baulk bucket: bucket -> '\x1b[31m%s\x1b[0m' not found\n", bucketName);
      return 1;
    }
    bela::FPrintF(stderr,
                  L"\x1b[34m%s\x1b[0m\n    \x1b[36m%s\x1b[0m\n    \x1b[36m%s (%s)\x1b[0m\n    \x1b[36m%d\x1b[0m\n",
                  bk.name, bk.description, bk.url, baulk::BucketObserveModeName(bk.mode), bk.weights);
    return 0;
  }
  try {
    const auto &bks = meta["bucket"];
    for (auto &b : bks) {
      auto desc = bela::ToWide(b["description"].get<std::string_view>());
      auto name = bela::ToWide(b["name"].get<std::string_view>());
      auto url = bela::ToWide(b["url"].get<std::string_view>());
      int weights = 100;
      if (auto it = b.find("weights"); it != b.end()) {
        weights = it.value().get<int>();
      }
      BucketObserveMode mode{BucketObserveMode::Github};
      if (auto it = b.find("mode"); it != b.end()) {
        if (auto mode_ = it.value().get<int>(); mode_ == 1) {
          mode = BucketObserveMode::Git;
        }
      }
      bela::FPrintF(stderr,
                    L"\x1b[34m%s\x1b[0m\n    \x1b[36m%s\x1b[0m\n    \x1b[36m%s (%s)\x1b[0m\n    \x1b[36m%d\x1b[0m\n",
                    name, desc, url, baulk::BucketObserveModeName(mode), weights);
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk bucket list: %s\n", e.what());
    return 1;
  }
  return 0;
}

int BucketModifier::Add(const baulk::Bucket &bucket, bool replace) {
  baulk::Bucket nbk = bucket;
  // Force setting mode
  if (bucket.url.starts_with(L"git@") || bucket.url.starts_with(L"ssh://")) {
    nbk.mode = baulk::BucketObserveMode::Git;
  }
  if (nbk.name.empty()) {
    if (std::vector<std::wstring_view> uvv = bela::StrSplit(nbk.url, bela::ByAnyChar(L"\\/:"), bela::SkipEmpty());
        uvv.size() > 2) {
      nbk.name = uvv[uvv.size() - 2];
    }
  }
  if (nbk.name.empty()) {
    nbk.name = L"anonymous";
  }
  baulk::DbgPrint(L"resolve bucket name %s", nbk.name);
  baulk::Bucket obk;
  if (GetBucket(nbk.name, obk)) {
    if (!replace) {
      bela::FPrintF(stderr,
                    L"baulk bucket: add bucket '\x1b[31m%s\x1b[0m' but it already exists\nYou can use the "
                    L"'\x1b[33m--replace\x1b[0m' parameter to replace it\n",
                    nbk.name);
      return 1;
    }
    if (nbk.description.empty()) {
      nbk.description = obk.description;
    }
    if (nbk.weights == 99 || nbk.weights == 0) {
      nbk.weights = obk.weights;
    }
    if (nbk.url.empty()) {
      nbk.url = obk.url;
    }
  }
  if (bucket.url.empty()) {
    bela::FPrintF(stderr, L"baulk bucket: add bucket '%s' url is blank\n", nbk.name);
    return 1;
  }
  if (nbk.description.empty()) {
    nbk.description = bela::StringCat(nbk.name, L" no description");
  }
  if (nbk.weights == 99 || nbk.weights == 0) {
    nbk.weights = 101;
  }
  bela::FPrintF(
      stderr,
      L"New bucket: \x1b[34m%s\x1b[0m\n    \x1b[36m%s\x1b[0m\n    \x1b[36m%s (%s)\x1b[0m\n    \x1b[36m%d\x1b[0m\n",
      nbk.name, nbk.description, nbk.url, baulk::BucketObserveModeName(nbk.mode), nbk.weights);

  try {
    auto buckets = nlohmann::json::array();
    if (auto it = meta.find("bucket"); it != meta.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::ToWide(bk["name"].get<std::string_view>());
        if (!bela::EqualsIgnoreCase(name, nbk.name)) {
          buckets.push_back(bk);
        }
      }
    }
    nlohmann::json jbk;
    jbk["description"] = bela::ToNarrow(nbk.description);
    jbk["name"] = bela::ToNarrow(nbk.name);
    jbk["url"] = bela::ToNarrow(nbk.url);
    jbk["mode"] = static_cast<int>(nbk.mode);
    jbk["weights"] = nbk.weights;
    buckets.emplace_back(std::move(jbk));
    meta["bucket"] = buckets;
    if (!Apply()) {
      bela::FPrintF(stderr, L"baulk bucket: add bucket %s apply error %s\n", nbk.name, ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk bucket: add bucket '%s' error %s\n", nbk.name, e.what());
  }
  bela::FPrintF(stderr, L"baulk bucket '%s' added\n", nbk.name);
  return 0;
}

bool PruneBucket(const baulk::Bucket &bucket) {
  auto bucketslock = bela::StringCat(baulk::BaulkRoot(), L"\\buckets\\buckets.lock.json");
  nlohmann::json newjson = nlohmann::json::array();
  [&]() -> bool {
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
          newjson.push_back(a);
          continue;
        }
        auto name = a["name"].get<std::string_view>();
        if (!bela::EqualsIgnoreCase(bucket.name, bela::ToWide(name))) {
          newjson.push_back(a);
        }
      }
    } catch (const std::exception &e) {
      bela::FPrintF(stderr, L"unable decode metadata. error: %s\n", e.what());
      return false;
    }
    return true;
  }();
  auto meta = newjson.dump(4);
  bela::error_code ec;
  if (!bela::io::WriteTextAtomic(meta, bucketslock, ec)) {
    bela::FPrintF(stderr, L"unable update %s error: %s\n", bucketslock, ec.message);
    return false;
  }
  auto bucketDir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName, L"\\", bucket.name);
  bela::fs::RemoveAll(bucketDir, ec);
  return true;
}

int BucketModifier::Del(const baulk::Bucket &bucket, bool forcemode) {
  if (bucket.name.empty()) {
    bela::FPrintF(stderr, L"baulk bucket delete: bucket name is blank\n");
    return 1;
  }
  bool deleted{false};
  try {
    auto buckets = nlohmann::json::array();
    if (auto it = meta.find("bucket"); it != meta.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::ToWide(bk["name"].get<std::string_view>());
        if (bela::EqualsIgnoreCase(name, bucket.name)) {
          deleted = true;
          continue;
        }
        buckets.push_back(bk);
      }
    }
    meta["bucket"] = buckets;
    if (!Apply()) {
      bela::FPrintF(stderr, L"baulk bucket delete: apply delete '%s' error %s\n", bucket.name, ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk bucket delete: %s\n", e.what());
  }
  if (!deleted) {
    bela::FPrintF(stderr, L"baulk bucket '\x1b[31m%s\x1b[0m' not exists\n", bucket.name);
    return 1;
  }
  if (forcemode) {
    PruneBucket(bucket);
  }
  bela::FPrintF(stderr, L"baulk bucket '\x1b[32m%s\x1b[0m' deleted\n", bucket.name);
  return 0;
}

void usage_bucket() {
  bela::FPrintF(stderr, LR"(Usage: baulk bucket <option> [bucket]...
Add, delete or list buckets.
  -N|--name           set bucket name
  -U|--url            set bucket url
  -W|--weights        set bucket weights, default: 100
  -M|--mode           set bucket mode (Git/Github)
  -D|--description    set bucket description
  -R|--replace        replace bucket with new attributes ('bucket add' support only)

Option:
  list, add, delete

Command Usage:
  baulk bucket list
  baulk bucket list BucketName

  baulk bucket add URL
  baulk bucket add BucketName URL
  baulk bucket add BucketName URL Weights
  baulk bucket add BucketName URL Weights Description

  baulk bucket delete BucketName

Example:
  baulk bucket add baulk git@github.com:baulk/bucket.git
  baulk bucket add baulk-mirror https://gitee.com/baulk/bucket.git -MGit -W102

)");
}

int cmd_bucket(const argv_t &argv) {
  BucketModifier bm;
  if (!bm.Initialize()) {
    bela::FPrintF(stderr, L"baulk bucket: load %s error: \x1b[31m%s\x1b[0m\n", baulk::BaulkProfile(),
                  bm.ErrorCode().message);
    return 1;
  }
  if (argv.empty()) {
    return bm.List();
  }
  baulk::cli::BaulkArgv ba(argv);
  ba.Add(L"verbose", baulk::cli::no_argument, L'V')
      .Add(L"name", baulk::cli::required_argument, L'N')
      .Add(L"url", baulk::cli::required_argument, L'U')
      .Add(L"weights", baulk::cli::required_argument, L'W')
      .Add(L"mode", baulk::cli::required_argument, L'M')
      .Add(L"description", baulk::cli::required_argument, L'D')
      .Add(L"replace", baulk::cli::no_argument, L'R')
      .Add(L"force", baulk::cli::no_argument, L'F');
  baulk::Bucket bucket;
  bela::error_code ec;
  bool replace{false};
  bool forcemode{false};
  auto ret = ba.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case L'V':
          baulk::IsDebugMode = true;
          break;
        case L'N':
          bucket.name = oa;
          break;
        case L'U':
          bucket.url = oa;
          break;
        case L'W':
          if (!bela::SimpleAtoi(oa, &bucket.weights)) {
            ec = bela::make_error_code(bela::ErrGeneral, L"unable parse weights: ", oa);
            return false;
          }
          break;
        case L'M':
          if (bela::EqualsIgnoreCase(oa, L"Github")) {
            bucket.mode = baulk::BucketObserveMode::Github;
            break;
          }
          if (bela::EqualsIgnoreCase(oa, L"Git")) {
            bucket.mode = baulk::BucketObserveMode::Git;
            break;
          }
          if (int m = 0; bela::SimpleAtoi(oa, &m)) {
            bucket.mode = static_cast<baulk::BucketObserveMode>(m);
            break;
          }
          ec = bela::make_error_code(bela::ErrGeneral, L"unable parse mode: ", oa);
          return false;
        case L'D':
          bucket.description = oa;
          break;
        case L'R':
          replace = true;
          break;
        case L'F':
          forcemode = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"baulk bucket: parse argv error \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  const auto ua = ba.Argv();
  if (ua.empty()) {
    return bm.List(bucket.name);
  }
  const auto sa = ua[0];
  size_t index = 1;
  if (bela::EqualsIgnoreCase(sa, L"list")) {
    if (bucket.name.empty() && index < ua.size()) {
      bucket.name = ua[index];
      index++;
    }
    for (; index < ua.size(); index++) {
      bela::FPrintF(stderr, L"baulk bucket: skip listing bucket '\x1b[33m%s\x1b[0m'\n", ua[index]);
    }
    return bm.List(bucket.name);
  }
  if (bela::EqualsIgnoreCase(sa, L"delete") || bela::EqualsIgnoreCase(sa, L"del")) {
    if (bucket.name.empty() && index < ua.size()) {
      bucket.name = ua[index];
      index++;
    }
    for (; index < ua.size(); index++) {
      bela::FPrintF(stderr, L"baulk bucket: skip deleting bucket '\x1b[33m%s\x1b[0m'\n", ua[index]);
    }
    return bm.Del(bucket, forcemode);
  }
  if (!bela::EqualsIgnoreCase(sa, L"add")) {
    bela::FPrintF(stderr, L"baulk bucket: unsupported subcommand '\x1b[31m%s\x1b[0m'\n", sa);
    return 1;
  }
  if (ua.size() == 2) {
    if (bucket.url.empty()) {
      bucket.url = ua[1];
    }
    return bm.Add(bucket, replace);
  }
  if (bucket.name.empty() && index < ua.size()) {
    bucket.name = ua[index];
    index++;
  }
  if (bucket.url.empty() && index < ua.size()) {
    bucket.url = ua[index];
    index++;
  }
  if (bucket.weights == 99 && index < ua.size()) {
    if (bela::SimpleAtoi(ua[index], &bucket.weights)) {
      index++;
    }
  }
  if (bucket.description.empty() && index < ua.size()) {
    bucket.description = ua[index];
    index++;
  }
  return bm.Add(bucket, replace);
}
} // namespace baulk::commands