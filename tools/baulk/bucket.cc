//
#include <bela/path.hpp>
#include <xml.hpp>
#include <jsonex.hpp>
#include <version.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "fs.hpp"
#include "net.hpp"
#include "decompress.hpp"

namespace baulk::bucket {
std::optional<std::wstring> BucketNewest(std::wstring_view bucketurl, bela::error_code &ec) {
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

bool BucketUpdate(std::wstring_view bucketurl, std::wstring_view name, std::wstring_view id,
                  bela::error_code &ec) {
  // https://github.com/baulk/bucket/archive/master.zip
  auto master = bela::StringCat(bucketurl, L"/archive/", id, L".zip");
  auto outdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName, L"\\temp");
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
  auto bucketdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName, L"\\", name);
  if (bela::PathExists(bucketdir)) {
    baulk::fs::PathRemove(bucketdir, ec);
  }
  if (MoveFileW(decompressdir.data(), bucketdir.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

std::optional<baulk::Package> PackageMeta(std::wstring_view pkgmeta, std::wstring_view pkgname,
                                          std::wstring_view bucket, bela::error_code &ec) {
  if (!bela::PathExists(pkgmeta)) {
    return std::nullopt;
  }
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, pkgmeta.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en);
    return std::nullopt;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  baulk::Package pkg;
  pkg.name = pkgname;
  pkg.bucket = bucket;
  try {
    auto j = nlohmann::json::parse(fd);
    baulk::json::JsonAssignor ja(j);
    pkg.description = ja.get("description");
    pkg.version = ja.get("version");
    // to lower
    pkg.extension = bela::AsciiStrToLower(ja.get("extension"));
    // load version
#if defined(_M_X64)
    // AMD64 support
    if (ja.get("url64", pkg.urls)) {
      pkg.checksum = ja.get("url64.hash");
    } else if (ja.get("url", pkg.urls)) {
      pkg.checksum = ja.get("url.hash");
    } else {
      ec = bela::make_error_code(1, pkgmeta, L" not yet ported.");
      return std::nullopt;
    }
    if (!ja.patharray("links64", pkg.links)) {
      ja.patharray("links", pkg.links);
    }
    if (!ja.patharray("launchers64", pkg.launchers)) {
      ja.patharray("launchers", pkg.launchers);
    }
#elif defined(_M_ARM64)
    // ARM64 support
    if (ja.get("urlarm64", pkg.urls)) {
      pkg.checksum = ja.get("urlarm64.hash");
    } else if (ja.get("url", pkg.urls)) {
      pkg.checksum = ja.get("url.hash");
    } else {
      ec = bela::make_error_code(1, pkgmeta, L" not yet ported.");
      return std::nullopt;
    }
    if (!ja.patharray("linksarm64", pkg.links)) {
      ja.patharray("links", pkg.links);
    }
    if (!ja.patharray("launchersarm64", pkg.launchers)) {
      ja.patharray("launchers", pkg.launchers);
    }
#else
    if (ja.get("url", pkg.urls)) {
      pkg.checksum = ja.get("url.hash");
    } else {
      ec = bela::make_error_code(1, pkgmeta, L" not yet ported.");
      return std::nullopt;
    }
    ja.patharray("links", pkg.links);
    ja.patharray("launchers", pkg.launchers);
#endif

  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, L"parse package meta json: ", bela::ToWide(e.what()));
    return std::nullopt;
  }

  return std::make_optional(std::move(pkg));
}

// installed package meta;
std::optional<baulk::Package> PackageLocalMeta(std::wstring_view pkgname, bela::error_code &ec) {
  auto pkglock = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks\\", pkgname, L".json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, pkglock.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en);
    return std::nullopt;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  baulk::Package pkg;
  pkg.name = pkgname;
  try {
    auto j = nlohmann::json::parse(fd);
    pkg.version = bela::ToWide(j["version"].get<std::string_view>());
    pkg.bucket = bela::ToWide(j["bucket"].get<std::string_view>());
    // must get bucket name
    pkg.weights = baulk::BaulkBucketWeights(pkg.bucket);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return std::nullopt;
  }
  return std::make_optional(std::move(pkg));
}

bool PackageUpdatableMeta(const baulk::Package &opkg, baulk::Package &pkg) {
  // initialize version from installed version
  baulk::version::version pkgversion(opkg.version);
  auto weights = opkg.weights;
  bool updated{false};
  auto bucketsdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName);

  for (const auto &bk : baulk::BaulkBuckets()) {
    auto pkgmeta = bela::StringCat(bucketsdir, L"\\", bk.name, L"\\bucket\\", opkg.name, L".json");
    bela::error_code ec;
    auto pkgN = PackageMeta(pkgmeta, opkg.name, bk.name, ec);
    if (!pkgN) {
      if (ec) {
        bela::FPrintF(stderr, L"Parse %s error: %s\n", pkgmeta, ec.message);
      }
      continue;
    }
    pkgN->bucket = bk.name;
    pkgN->weights = bk.weights;
    baulk::version::version newversion(pkgN->version);
    // compare version newversion is > oldversion
    // newversion == oldversion and strversion not equail compare weights
    if (newversion > pkgversion || (newversion == pkgversion && weights < pkgN->weights)) {
      pkg = std::move(*pkgN);
      pkgversion = newversion;
      weights = pkgN->weights;
      updated = true;
    }
  }
  return updated;
}
// package metadata
std::optional<baulk::Package> PackageMetaEx(std::wstring_view pkgname, bela::error_code &ec) {
  baulk::version::version pkgversion; // 0.0.0.0
  baulk::Package pkg;
  auto bucketsdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName);
  size_t pkgsame = 0;
  for (const auto &bk : baulk::BaulkBuckets()) {
    auto pkgmeta = bela::StringCat(bucketsdir, L"\\", bk.name, L"\\bucket\\", pkgname, L".json");
    bela::error_code ec;
    auto pkgN = PackageMeta(pkgmeta, pkgname, bk.name, ec);
    if (!pkgN) {
      if (ec) {
        bela::FPrintF(stderr, L"Parse %s error: %s\n", pkgmeta, ec.message);
      }
      continue;
    }
    pkgsame++;
    pkgN->bucket = bk.name;
    pkgN->weights = bk.weights;
    baulk::version::version newversion(pkgN->version);
    // compare version newversion is > oldversion
    // newversion == oldversion and strversion not equail compare weights
    if (newversion > pkgversion || (newversion == pkgversion && pkg.weights < pkgN->weights)) {
      pkg = std::move(*pkgN);
      pkgversion = newversion;
    }
  }
  if (pkgsame == 0) {
    ec = bela::make_error_code(1, L"'", pkgname, L"' not yet ported.");
    return std::nullopt;
  }
  return std::make_optional(std::move(pkg));
}

bool PackageIsUpdatable(std::wstring_view pkgname, baulk::Package &pkg) {
  bela::error_code ec;
  auto opkg = PackageLocalMeta(pkgname, ec);
  if (!opkg) {
    return false;
  }
  return PackageUpdatableMeta(*opkg, pkg);
}

} // namespace baulk::bucket
