//
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/semver.hpp>
#include <baulk/json_utils.hpp>
#include <xml.hpp>

#include "baulk.hpp"
#include "bucket.hpp"
#include <baulk/fs.hpp>
#include "net.hpp"
#include "decompress.hpp"

namespace baulk::bucket {
std::optional<std::wstring> BucketNewestWithGithub(std::wstring_view bucketurl, bela::error_code &ec) {
  // default branch atom
  auto rss = bela::StringCat(bucketurl, L"/commits.atom");
  baulk::DbgPrint(L"Fetch RSS %s", rss);
  auto resp = baulk::net::RestGet(rss, ec);
  if (!resp) {
    return std::nullopt;
  }
  auto doc = baulk::xml::parse_string(resp->body, ec);
  if (!doc) {
    return std::nullopt;
  }
  std::string_view title{doc->child("feed").child("title").text().as_string()};
  baulk::DbgPrint(L"bucket commits: %s", title);
  // first entry child
  auto entry = doc->child("feed").child("entry");
  std::string_view id{entry.child("id").text().as_string()};
  if (auto pos = id.find('/'); pos != std::string_view::npos) {
    return std::make_optional(bela::encode_into<char, wchar_t>(id.substr(pos + 1)));
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"bucket invaild id: ", bela::encode_into<char, wchar_t>(id));
  return std::nullopt;
}
// git ls-remote filter HEAD
std::optional<std::wstring> BucketRepoNewest(std::wstring_view giturl, bela::error_code &ec) {
  bela::process::Process process;
  if (process.Capture(L"git", L"ls-remote", giturl, L"HEAD") != 0) {
    if (ec = process.ErrorCode(); !ec) {
      ec = bela::make_error_code(process.ExitCode(), L"git exit with: ", process.ExitCode());
    }
    return std::nullopt;
  }
  constexpr std::string_view head = "HEAD";
  std::vector<std::string_view> lines =
      bela::narrow::StrSplit(process.Out(), bela::narrow::ByChar('\n'), bela::narrow::SkipEmpty());
  for (auto line : lines) {
    std::vector<std::string_view> kv =
        bela::narrow::StrSplit(line, bela::narrow::ByAnyChar("\t "), bela::narrow::SkipEmpty());
    if (kv.size() == 2 && kv[1] == head) {
      return std::make_optional(bela::encode_into<char, wchar_t>(kv[0]));
    }
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"not found HEAD: ", giturl);
  return std::nullopt;
}

//
std::optional<std::wstring> BucketNewest(const baulk::Bucket &bucket, bela::error_code &ec) {
  if (bucket.mode == baulk::BucketObserveMode::Github) {
    return BucketNewestWithGithub(bucket.url, ec);
  }
  if (bucket.mode != baulk::BucketObserveMode::Git) {
    ec = bela::make_error_code(bela::ErrGeneral, L"Unsupported bucket mode: ", static_cast<int>(bucket.mode));
    return std::nullopt;
  }
  return BucketRepoNewest(bucket.url, ec);
}

bool BucketRepoUpdate(const baulk::Bucket &bucket, bela::error_code &ec) {
  auto bucketDir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName, L"\\", bucket.name);
  bela::process::Process process;
  if (bela::PathExists(bucketDir)) {
    process.Chdir(bucketDir);
    // Force update
    if (process.Execute(L"git", L"pull", L"--force", bucket.url) != 0) {
      return false;
    }
    return true;
  }
  if (process.Execute(L"git", L"clone", L"--depth=1", bucket.url, bucketDir) != 0) {
    return false;
  }
  return true;
}

bool BucketUpdate(const baulk::Bucket &bucket, std::wstring_view id, bela::error_code &ec) {
  if (bucket.mode == baulk::BucketObserveMode::Git) {
    return BucketRepoUpdate(bucket, ec);
  }
  if (bucket.mode != baulk::BucketObserveMode::Github) {
    ec = bela::make_error_code(bela::ErrGeneral, L"Unsupported bucket mode: ", static_cast<int>(bucket.mode));
    return false;
  }
  // https://github.com/baulk/bucket/archive/master.zip
  auto master = bela::StringCat(bucket.url, L"/archive/", id, L".zip");
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
    bela::fs::ForceDeleteFile(*outfile, ec_);
  });
  auto decompressdir = bela::StringCat(outdir, L"\\", bucket.name);
  if (!baulk::zip::Decompress(*outfile, decompressdir, ec)) {
    return false;
  }
  baulk::standard::Regularize(decompressdir);
  auto bucketdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName, L"\\", bucket.name);
  if (bela::PathExists(bucketdir)) {
    bela::fs::ForceDeleteFolders(bucketdir, ec);
  }
  if (MoveFileW(decompressdir.data(), bucketdir.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

std::optional<baulk::Package> PackageMeta(std::wstring_view pkgmeta, std::wstring_view pkgname,
                                          std::wstring_view bucket, bela::error_code &ec) {
  auto pkj = baulk::json::parse_file(pkgmeta, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring{pkgname},
      .description = jv.fetch("description"),
      .version = jv.fetch("version"),
      .bucket = std::wstring{bucket},
      .extension = bela::AsciiStrToLower(jv.fetch("extension")), // to lower
      .rename = jv.fetch("rename"),
      .homepage = jv.fetch("homepage"),
      .notes = jv.fetch("notes"),
      .license = jv.fetch("license"),
  };
  jv.fetch_strings_checked("suggest", pkg.suggest);
  jv.fetch_paths_checked("force_delete", pkg.forceDeletes);

#if defined(_M_X64)
  // x64
  if (jv.fetch_strings_checked("url64", pkg.urls)) {
    pkg.checksum = jv.fetch("url64.hash");
  } else if (jv.fetch_strings_checked("url", pkg.urls)) {
    pkg.checksum = jv.fetch("url.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgmeta, L" not yet port to x64 platform.");
    return std::nullopt;
  }
  if (!jv.fetch_paths_checked("links64", pkg.links)) {
    jv.fetch_paths_checked("links", pkg.links);
  }
  if (!jv.fetch_paths_checked("launchers64", pkg.launchers)) {
    jv.fetch_paths_checked("launchers", pkg.launchers);
  }
#elif defined(_M_ARM64)
  // ARM64 support
  if (jv.fetch_strings_checked("urlarm64", pkg.urls)) {
    pkg.checksum = jv.fetch("urlarm64.hash");
  } else if (jv.fetch_strings_checked("url", pkg.urls)) {
    pkg.checksum = jv.fetch("url.hash");
  } else if (jv.fetch_strings_checked("url64", pkg.urls)) {
    pkg.checksum = jv.fetch("url64.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgmeta, L" not yet port to ARM64 platform.");
    return std::nullopt;
  }
  if (!jv.fetch_paths_checked("linksarm64", pkg.links)) {
    if (!jv.fetch_paths_checked("links", pkg.links)) {
      jv.fetch_paths_checked("links64", pkg.links);
    }
  }
  if (!jv.fetch_paths_checked("launchersarm64", pkg.launchers)) {
    if (!jv.fetch_paths_checked("launchers", pkg.launchers)) {
      jv.fetch_paths_checked("launchers", pkg.links);
    }
  }
#else
  if (jv.fetch_strings_checked("url", pkg.urls)) {
    pkg.checksum = jv.fetch("url.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgmeta, L" not yet ported.");
    return std::nullopt;
  }
  jv.fetch_paths_checked("links", pkg.links);
  jv.fetch_paths_checked("launchers", pkg.launchers);
#endif
  if (auto sv = jv.subview("venv"); sv) {
    DbgPrint(L"pkg '%s' support virtual env\n", pkg.name);
    pkg.venv.category = sv->fetch("category");
    sv->fetch_paths_checked("path", pkg.venv.paths);
    sv->fetch_paths_checked("include", pkg.venv.includes);
    sv->fetch_paths_checked("lib", pkg.venv.libs);
    sv->fetch_paths_checked("mkdir", pkg.venv.mkdirs);
    sv->fetch_strings_checked("env", pkg.venv.envs);
    sv->fetch_strings_checked("dependencies", pkg.venv.dependencies);
  }
  return std::make_optional(std::move(pkg));
}

// installed package meta;
std::optional<baulk::Package> PackageLocalMeta(std::wstring_view pkgname, bela::error_code &ec) {
  auto pkglock = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks\\", pkgname, L".json");
  auto pkj = baulk::json::parse_file(pkglock, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring(pkgname),
      .version = jv.fetch("version"),
      .bucket = jv.fetch("bucket"),
  };
  jv.fetch_paths_checked("force_delete", pkg.forceDeletes);
  if (auto sv = jv.subview("venv"); sv) {
    pkg.venv.category = sv->fetch("category");
  }
  pkg.weights = baulk::BaulkBucketWeights(pkg.bucket);
  return std::make_optional(std::move(pkg));
}

bool PackageUpdatableMeta(const baulk::Package &opkg, baulk::Package &pkg) {
  // initialize version from installed version
  bela::version pkgversion(opkg.version);
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
    bela::version newversion(pkgN->version);
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
  ec.clear();
  bela::version pkgversion; // 0.0.0.0
  baulk::Package pkg;
  auto bucketsdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BucketsDirName);
  size_t pkgsame = 0;
  for (const auto &bk : baulk::BaulkBuckets()) {
    auto pkgmeta = bela::StringCat(bucketsdir, L"\\", bk.name, L"\\bucket\\", pkgname, L".json");
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
    bela::version newversion(pkgN->version);
    // compare version newversion is > oldversion
    // newversion == oldversion and strversion not equail compare weights
    if (newversion > pkgversion || (newversion == pkgversion && pkg.weights < pkgN->weights)) {
      pkg = std::move(*pkgN);
      pkgversion = newversion;
    }
  }
  if (pkgsame == 0) {
    ec = bela::make_error_code(ErrPackageNotYetPorted, L"'", pkgname, L"' not yet ported.");
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
