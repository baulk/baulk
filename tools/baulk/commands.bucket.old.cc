// bucket
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <json.hpp>
#include "baulk.hpp"
#include "commands.hpp"
#include "hash.hpp"
#include "fs.hpp"

namespace baulk::commands {

inline bool BaulkLoad(nlohmann::json &json, bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, baulk::BaulkProfile().data(), L"rb"); eo != 0) {
    ec = bela::make_stdc_error_code(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    json = nlohmann::json::parse(fd, nullptr, true, true);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::ToWide(e.what()));
    return false;
  }
  return true;
}

inline bool BaulkStore(std::string_view jsontext, bela::error_code &ec) {
  return bela::io::WriteTextAtomic(jsontext, baulk::BaulkProfile(), ec);
}

int ListBucket() {
  bela::FPrintF(stderr, L"Baulk found \x1b[32m%d\x1b[0m buckets\n", baulk::BaulkBuckets().size());
  for (const auto &bk : baulk::BaulkBuckets()) {
    bela::FPrintF(stderr,
                  L"\x1b[34m%s\x1b[0m\n    \x1b[36m%s\x1b[0m\n    \x1b[36m%s (%s)\x1b[0m\n    \x1b[36m%d\x1b[0m\n",
                  bk.name, bk.description, bk.url, baulk::BucketObserveModeName(bk.mode), bk.weights);
  }
  return 0;
}

int AddBucket(const argv_t &argv) {
  if (argv.size() < 2) {
    bela::FPrintF(stderr, L"usage: baulk bucket add \x1b[33mURL\x1b[0m\n       baulk bucket add \x1b[33mBucketName "
                          L"URL\x1b[0m\n       baulk bucket add \x1b[33mBucketName URL Weights\x1b[0m\n       baulk "
                          L"bucket add \x1b[33mBucketName URL Weights Description\x1b[0m\n");
    return 1;
  }
  std::wstring description = L"description";
  std::wstring bucketName = L"anonymous";
  std::wstring bucketURL;
  int weights = 101;
  // add BucketName URL Weights
  if (argv.size() > 4) {
    description = argv[4];
  }
  if (argv.size() > 3) {
    if (!bela::SimpleAtoi(argv[3], &weights)) {
      bela::FPrintF(stderr, L"baulk bucket add bad weights: \x1b[31m%s\x1b[0m\n", argv[3]);
      return 1;
    }
  }
  if (argv.size() > 2) {
    bucketName = argv[1];
    bucketURL = argv[2];
  } else {
    bucketURL = argv[1];
    auto uvv = bela::SplitPath(bucketURL);
    if (uvv.size() > 2) {
      bucketName = uvv[uvv.size() - 2];
    }
  }
  for (const auto &bk : baulk::BaulkBuckets()) {
    if (bela::EqualsIgnoreCase(bk.name, bucketName)) {
      bela::FPrintF(stderr, L"Bucket '%s' exists\n", bucketName);
      return 1;
    }
  }
  bela::FPrintF(stderr, L"Add new bucket '%s' url: %s\n", bucketName, bucketURL);
  bela::error_code ec;
  nlohmann::json json;
  if (!BaulkLoad(json, ec)) {
    bela::FPrintF(stderr, L"unable load baulk profile: %s\n", ec.message);
    return 1;
  }

  try {
    auto it = json.find("bucket");
    auto buckets = nlohmann::json::array();
    nlohmann::json bucket;
    bucket["description"] = bela::ToNarrow(description);
    bucket["name"] = bela::ToNarrow(bucketName);
    bucket["url"] = bela::ToNarrow(bucketURL);
    bucket["weights"] = weights;
    buckets.emplace_back(std::move(bucket));
    if (it != json.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::ToWide(bk["name"].get<std::string_view>());
        buckets.push_back(bk);
      }
    }
    json["bucket"] = buckets;
    auto jsontext = json.dump(4);
    if (!BaulkStore(jsontext, ec)) {
      bela::FPrintF(stderr, L"unable store baulk buckets: %s\n", ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable store buckets: %s\n", e.what());
    return 1;
  }
  bela::FPrintF(stderr, L"baulk '%s' added\n", bucketName);
  return 0;
}

int ConvertBucket(const argv_t &argv) {
  if (argv.size() < 4) {
    bela::FPrintF(stderr, L"usage: baulk bucket convert '\x1b[31mBucketName\x1b[0m' '\x1b[31mURL\x1b[0m' "
                          L"'\x1b[31mmode\x1b (Github/Git)[0m'\n");
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk bucket convert: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }

  nlohmann::json json;
  if (!BaulkLoad(json, ec)) {
    bela::FPrintF(stderr, L"unable load baulk profile: %s\n", ec.message);
    return 1;
  }

  try {
    auto it = json.find("bucket");
    auto buckets = nlohmann::json::array();
    if (it != json.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::ToWide(bk["name"].get<std::string_view>());
        if (!bela::EqualsIgnoreCase(name, argv[1])) {
          buckets.push_back(bk);
          continue;
        }
        nlohmann::json newjson;
        newjson["name"] = name;
        newjson["weights"] = 100;
        newjson["url"] = argv[2];
      }
    }
    json["bucket"] = buckets;
    auto jsontext = json.dump(4);
    if (!BaulkStore(jsontext, ec)) {
      bela::FPrintF(stderr, L"unable store baulk buckets: %s\n", ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable store buckets: %s\n", e.what());
    return 1;
  }
  bela::FPrintF(stderr, L"baulk convert '%s' success\n", argv[1]);
  return 0;
}

int DeleteBucket(const argv_t &argv) {
  if (argv.size() < 2) {
    bela::FPrintF(stderr, L"usage: baulk bucket delete '\x1b[31mBucketName\x1b[0m'\n");
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk bucket delete: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }

  nlohmann::json json;
  if (!BaulkLoad(json, ec)) {
    bela::FPrintF(stderr, L"unable load baulk profile: %s\n", ec.message);
    return 1;
  }

  try {
    auto it = json.find("bucket");
    auto buckets = nlohmann::json::array();
    if (it != json.end()) {
      for (const auto &bk : it.value()) {
        auto name = bela::ToWide(bk["name"].get<std::string_view>());
        if (!bela::EqualsIgnoreCase(name, argv[1])) {
          buckets.push_back(bk);
        }
      }
    }
    json["bucket"] = buckets;
    auto jsontext = json.dump(4);
    if (!BaulkStore(jsontext, ec)) {
      bela::FPrintF(stderr, L"unable store baulk buckets: %s\n", ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable store buckets: %s\n", e.what());
    return 1;
  }
  bela::FPrintF(stderr, L"baulk '%s' deleted\n", argv[1]);
  return 0;
}

void usage_bucket() {
  bela::FPrintF(stderr, LR"(Usage: baulk bucket <option> [bucket]...
Add, delete or list buckets.

Option:
  list add delete

Example:
  baulk bucket list
  baulk bucket list BucketName

  baulk bucket add URL
  baulk bucket add BucketName URL
  baulk bucket add BucketName URL Weights
  baulk bucket add BucketName URL Weights Description

  baulk bucket delete BucketName

  baulk bucket convert BucketName URL mode

)");
}

int cmd_bucket(const argv_t &argv) {
  if (argv.empty()) {
    return ListBucket();
  }
  if (bela::EqualsIgnoreCase(argv[0], L"add")) {
    return AddBucket(argv);
  }
  if (bela::EqualsIgnoreCase(argv[0], L"delete")) {
    return DeleteBucket(argv);
  }
  if (bela::EqualsIgnoreCase(argv[0], L"list")) {
    return ListBucket();
  }
  if (bela::EqualsIgnoreCase(argv[0], L"convert")) {
    return ConvertBucket(argv);
  }
  bela::FPrintF(stderr, L"baulk bucket: unsupported subcommand '\x1b[31m%s\x1b[0m'\n", argv[0]);
  return 1;
}
} // namespace baulk::commands