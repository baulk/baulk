// bucket
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <baulk/hash.hpp>
#include <baulk/fs.hpp>
#include <baulk/argv.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/vfs.hpp>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {

inline void displayBucket(const Bucket &bk) {
  bela::FPrintF(stderr,
                L"bucket:      \x1b[34m%v\x1b[0m\n"    //
                L"description: \x1b[36m%v\x1b[0m\n"    //
                L"url:         \x1b[36m%v\x1b[0m\n"    //
                L"variant:     \x1b[36m%v\x1b[0m\n"    //
                L"mode:        \x1b[36m%v\x1b[0m\n"    //
                L"weights:     \x1b[36m%v\x1b[0m\n",   //
                bk.name,                               //
                bk.description,                        //
                bk.url,                                //
                baulk::BucketVariantName(bk.variant),  //
                baulk::BucketObserveModeName(bk.mode), //
                bk.weights);
}

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
  [[nodiscard]] const bela::error_code &ErrorCode() const { return ec; }

private:
  bela::error_code ec;
  nlohmann::json meta;
};

bool BucketModifier::Initialize() {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, baulk::Profile().data(), L"rb"); eo != 0) {
    ec = bela::make_error_code_from_errno(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    meta = nlohmann::json::parse(fd, nullptr, true, true);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  return true;
}

bool BucketModifier::Apply() {
  return bela::io::AtomicWriteText(baulk::Profile(), bela::io::as_bytes<char>(meta.dump(4)), ec);
}

bool BucketModifier::GetBucket(std::wstring_view bucketName, baulk::Bucket &bucket) {
  try {
    const auto &bks = meta["bucket"];
    for (const auto &b : bks) {
      auto name = bela::encode_into<char, wchar_t>(b["name"].get<std::string_view>());
      if (!bela::EqualsIgnoreCase(bucketName, name)) {
        continue;
      }
      bucket.name = name;
      bucket.description = bela::encode_into<char, wchar_t>(b["description"].get<std::string_view>());
      bucket.url = bela::encode_into<char, wchar_t>(b["url"].get<std::string_view>());
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
    displayBucket(bk);
    return 0;
  }
  try {
    const auto &bks = meta["bucket"];
    for (const auto &b : bks) {
      auto desc = bela::encode_into<char, wchar_t>(b["description"].get<std::string_view>());
      auto name = bela::encode_into<char, wchar_t>(b["name"].get<std::string_view>());
      auto url = bela::encode_into<char, wchar_t>(b["url"].get<std::string_view>());
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
      BucketVariant variant{BucketVariant::Native};
      if (auto it = b.find("variant"); it != b.end()) {
        variant = static_cast<BucketVariant>(it.value().get<int>());
      }
      Bucket bucket(desc, name, url, weights, mode, variant);
      displayBucket(bucket);
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
    if (nbk.weights == 0) {
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
  if (nbk.weights == 0) {
    nbk.weights = 99;
  }
  displayBucket(nbk);
  try {
    auto buckets = nlohmann::json::array();
    if (auto it = meta.find("bucket"); it != meta.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::encode_into<char, wchar_t>(bk["name"].get<std::string_view>());
        if (!bela::EqualsIgnoreCase(name, nbk.name)) {
          buckets.push_back(bk);
        }
      }
    }
    nlohmann::json jbk;
    jbk["description"] = bela::encode_into<wchar_t, char>(nbk.description);
    jbk["name"] = bela::encode_into<wchar_t, char>(nbk.name);
    jbk["url"] = bela::encode_into<wchar_t, char>(nbk.url);
    jbk["mode"] = static_cast<int>(nbk.mode);
    jbk["weights"] = nbk.weights;
    jbk["variant"] = nbk.variant;
    buckets.emplace_back(std::move(jbk));
    meta["bucket"] = buckets;
    if (!Apply()) {
      bela::FPrintF(stderr, L"baulk bucket: add bucket %s apply error %s\n", nbk.name, ec);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk bucket: add bucket '%s' error %s\n", nbk.name, e.what());
  }
  bela::FPrintF(stderr, L"baulk bucket '%s' added\n", nbk.name);
  return 0;
}

bool PruneBucket(const baulk::Bucket &bucket) {
  auto lockfile = bela::StringCat(vfs::AppBuckets(), L"\\buckets.lock.json");
  nlohmann::json newjson = nlohmann::json::array();
  [&]() -> bool {
    FILE *fd = nullptr;
    if (auto en = _wfopen_s(&fd, lockfile.data(), L"rb"); en != 0) {
      if (en == ENOENT) {
        return true;
      }
      auto ec = bela::make_error_code_from_errno(en);
      bela::FPrintF(stderr, L"unable load %s error: %s\n", lockfile, ec);
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
        if (!bela::EqualsIgnoreCase(bucket.name, bela::encode_into<char, wchar_t>(name))) {
          newjson.push_back(a);
        }
      }
    } catch (const std::exception &e) {
      bela::FPrintF(stderr, L"unable decode metadata. error: %s\n", e.what());
      return false;
    }
    return true;
  }();
  bela::error_code ec;
  if (!bela::io::AtomicWriteText(lockfile, bela::io::as_bytes<char>(newjson.dump(4)), ec)) {
    bela::FPrintF(stderr, L"unable update %s error: %s\n", lockfile, ec);
    return false;
  }
  auto bucketDir = bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name);
  bela::fs::ForceDeleteFolders(bucketDir, ec);
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
        auto name = bela::encode_into<char, wchar_t>(bk["name"].get<std::string_view>());
        if (bela::EqualsIgnoreCase(name, bucket.name)) {
          deleted = true;
          continue;
        }
        buckets.push_back(bk);
      }
    }
    meta["bucket"] = buckets;
    if (!Apply()) {
      bela::FPrintF(stderr, L"baulk bucket delete: apply delete '%s' error %s\n", bucket.name, ec);
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
  -W|--weights        set bucket weights, default: 99 (baulk is 100)
  -M|--mode           set bucket mode (Git/Github)
  -A|--variant        set bucket variant (native/scoop)
  -D|--description    set bucket description
  -R|--replace        replace bucket with new attributes ('bucket add' support only)

Option:
  list, add, delete, help

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
  baulk bucket add scoop git@github.com:ScoopInstaller/Main.git -MGit -Ascoop

)");
}

int cmd_bucket(const argv_t &argv) {
  BucketModifier bm;
  if (!bm.Initialize()) {
    bela::FPrintF(stderr, L"baulk bucket: load %s error: \x1b[31m%s\x1b[0m\n", baulk::Profile(),
                  bm.ErrorCode().message);
    return 1;
  }
  if (argv.empty()) {
    return bm.List();
  }
  baulk::cli::ParseArgv pa(argv);
  pa.Add(L"verbose", baulk::cli::no_argument, L'V')
      .Add(L"name", baulk::cli::required_argument, L'N')
      .Add(L"url", baulk::cli::required_argument, L'U')
      .Add(L"weights", baulk::cli::required_argument, L'W')
      .Add(L"mode", baulk::cli::required_argument, L'M')
      .Add(L"variant", baulk::cli::required_argument, L'A')
      .Add(L"description", baulk::cli::required_argument, L'D')
      .Add(L"replace", baulk::cli::no_argument, L'R')
      .Add(L"force", baulk::cli::no_argument, L'F');
  baulk::Bucket bucket;
  bela::error_code ec;
  bool replace{false};
  bool forcemode{false};
  auto ret = pa.Execute(
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
          if (bela::EqualsIgnoreCase(oa, L"github") || bela::EqualsIgnoreCase(oa, L"github-zip")) {
            bucket.mode = baulk::BucketObserveMode::Github;
            break;
          }
          if (bela::EqualsIgnoreCase(oa, L"git")) {
            bucket.mode = baulk::BucketObserveMode::Git;
            break;
          }
          if (int m = 0; bela::SimpleAtoi(oa, &m)) {
            bucket.mode = static_cast<baulk::BucketObserveMode>(m);
            break;
          }
          ec = bela::make_error_code(bela::ErrGeneral, L"unable parse mode: ", oa);
          return false;
        case L'A':
          if (bela::EqualsIgnoreCase(oa, L"native")) {
            bucket.variant = baulk::BucketVariant::Native;
            break;
          }
          if (bela::EqualsIgnoreCase(oa, L"scoop")) {
            bucket.variant = baulk::BucketVariant::Scoop;
            break;
          }
          if (int v = 0; bela::SimpleAtoi(oa, &v)) {
            bucket.variant = static_cast<baulk::BucketVariant>(v);
            break;
          }
          ec = bela::make_error_code(bela::ErrGeneral, L"unable parse variant: ", oa);
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
    bela::FPrintF(stderr, L"baulk bucket: parse argv error \x1b[31m%s\x1b[0m\n", ec);
    return 1;
  }
  const auto ua = pa.Argv();
  if (ua.empty()) {
    return bm.List(bucket.name);
  }
  const auto sa = ua[0];
  size_t index = 1;
  if (bela::EqualsIgnoreCase(sa, L"help")) {
    usage_bucket();
    return 0;
  }
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