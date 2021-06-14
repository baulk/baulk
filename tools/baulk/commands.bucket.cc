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
  bool List();
  bool Add(const baulk::Bucket &bucket);
  bool Del(const baulk::Bucket &bucket);
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

bool BucketModifier::List() {
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
    ec = bela::make_error_code(bela::ErrGeneral, L"baulk bucket list: ", bela::ToWide(e.what()));
    return false;
  }
  return true;
}

bool BucketModifier::Add(const baulk::Bucket &bucket) {
  //
  return true;
}
bool BucketModifier::Del(const baulk::Bucket &bucket) {
  //
  return true;
}

void usage_bucket() {
  bela::FPrintF(stderr, LR"(Usage: baulk bucket <option> [bucket]...

)");
}

int cmd_bucket(const argv_t &argv) {
  BucketModifier bm;
  if (!bm.Initialize()) {
    bela::FPrintF(stderr, L"baulk bucket: load bucket meta error %s\n", bm.ErrorCode().message);
    return 1;
  }
  if (argv.empty()) {
    if (!bm.List()) {
      bela::FPrintF(stderr, L"baulk bucket: list buckets error %s\n", bm.ErrorCode().message);
    }
    return 0;
  }
  baulk::cli::BaulkArgv ba(argv);
  ba.Add(L"name", baulk::cli::required_argument, L'N')
      .Add(L"url", baulk::cli::required_argument, L'U')
      .Add(L"weights", baulk::cli::required_argument, L'W')
      .Add(L"mode", baulk::cli::required_argument, L'M')
      .Add(L"description", baulk::cli::required_argument, L'D');
  baulk::Bucket bucket;
  bela::error_code ec;
  auto ret = ba.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case L'N':
          bucket.name = oa;
          break;
        case L'U':
          bucket.url = oa;
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
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"baulk bucket: parse argv error %s\n", ec.message);
    return 1;
  }
  const auto ua = ba.Argv();
  if (ua.empty()) {

    return 0;
  }
  return 0;
}
} // namespace baulk::commands