//
#include <bela/path.hpp>
#include <xml.hpp>
#include <jsonex.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "fs.hpp"
#include "net.hpp"
#include "decompress.hpp"

namespace baulk::bucket {
//
std::optional<std::wstring> BucketNewest(std::wstring_view bucketurl,
                                         bela::error_code &ec) {
  auto rss = bela::StringCat(bucketurl, L"/commits/master.atom");
  baulk::DbgPrint(L"Fetch RSS %s", rss);
  auto resp = baulk::net::RestGet(rss, ec);
  if (!resp) {
    return std::nullopt;
  }
  auto doc = baulk::xml::parse_string(resp->body, ec);
  if (!doc) {
    return std::nullopt;
  }
  // first entry child
  auto entry = doc->child("feed").child("entry");
  std::string_view id{entry.child("id").text().as_string()};
  if (auto pos = id.find('/'); pos != std::string_view::npos) {
    return std::make_optional(bela::ToWide(id.substr(pos + 1)));
  }
  ec = bela::make_error_code(1, L"bucket invaild id: ", bela::ToWide(id));
  return std::nullopt;
}

bool BucketUpdate(std::wstring_view bucketurl, std::wstring_view name,
                  std::wstring_view id, bela::error_code &ec) {
  // https://github.com/baulk/bucket/archive/master.zip
  auto master = bela::StringCat(bucketurl, L"/archive/", id, L".zip");
  auto outdir = bela::StringCat(baulk::BaulkRoot(), L"\\",
                                baulk::BucketsDirName, L"\\temp");
  if (!bela::PathExists(outdir) && !baulk::fs::MakeDir(outdir, ec)) {
    return false;
  }
  auto outfile = baulk::net::WinGet(master, outdir, true, ec);
  if (!outfile) {
    return false;
  }
  auto deleter = bela::finally([&] {
    bela::error_code ec_;
    baulk::fs::PathRemove(*outfile, ec_);
  });
  auto decompressdir = bela::StringCat(outdir, L"\\", name);
  if (!baulk::zip::Decompress(*outfile, decompressdir, ec)) {
    return false;
  }
  baulk::standard::Regularize(decompressdir);
  auto bucketdir = bela::StringCat(baulk::BaulkRoot(), L"\\",
                                   baulk::BucketsDirName, L"\\", name);
  if (bela::PathExists(bucketdir)) {
    baulk::fs::PathRemove(bucketdir, ec);
  }
  if (MoveFileW(decompressdir.data(), bucketdir.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

// search all available packages
bool Scanner::Initialize() {
  auto bucketsDir =
      bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName);
  for (const auto &p : std::filesystem::directory_iterator(bucketsDir)) {
    auto dir = p.path().wstring();
    auto bk = bela::StringCat(dir, L"\\bucket");
    if (bela::PathExists(bk)) {
      buckets.emplace_back(std::move(dir));
    }
  }
  return true;
}

bool Scanner::IsUpdatable(std::wstring_view pkgname, std::wstring_view path,
                          std::wstring &version) {

  return false;
}

} // namespace baulk::bucket
