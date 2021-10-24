//
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/semver.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/vfs.hpp>
#include <baulk/net.hpp>
#include <baulk/fs.hpp>
#include <xml.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "decompress.hpp"

namespace baulk::bucket {
// BucketNewestWithGithub github archive style bucket check latest
std::optional<std::wstring> BucketNewestWithGithub(std::wstring_view bucketurl, bela::error_code &ec) {
  // default branch atom
  auto rss = bela::StringCat(bucketurl, L"/commits.atom");
  baulk::DbgPrint(L"Fetch RSS %s", rss);
  auto resp = baulk::net::RestGet(rss, ec);
  if (!resp) {
    return std::nullopt;
  }
  auto doc = baulk::xml::parse_string(resp->Content(), ec);
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

// BucketRepoNewest git style: git ls-remote filter HEAD
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

// BucketNewest
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
  auto bucketDir = bela::StringCat(baulk::vfs::AppBuckets(), L"\\", bucket.name);
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
  auto temp = baulk::vfs::AppTemp();
  if (!baulk::fs::MakeDir(temp, ec)) {
    return false;
  }
  std::optional<std::wstring> saveFile;
  bela::FPrintF(stderr, L"baulk: download \x1b[36m%s\x1b[0m metadata\nurl: \x1b[36m%s\x1b[0m\n", bucket.name, master);
  for (int i = 0; i < 4; i++) {
    ec.clear();
    if (i != 0) {
      bela::FPrintF(stderr, L"baulk: download \x1b[36m%s\x1b[0m metadata. retries: \x1b[33m%d\x1b[0m", bucket.name, i);
    }
    if (saveFile = baulk::net::WinGet(master, temp, L"", true, ec); saveFile) {
      break;
    }
  }
  if (!saveFile) {
    return false;
  }

  auto deleter = bela::finally([&] {
    bela::error_code ec_;
    bela::fs::ForceDeleteFile(*saveFile, ec_);
  });

  auto extractDir = bela::StringCat(temp, L"\\", bucket.name);
  if (!baulk::zip::Decompress(*saveFile, extractDir, ec)) {
    return false;
  }
  baulk::standard::Regularize(extractDir);
  auto bucketdir = bela::StringCat(baulk::vfs::AppBuckets(), L"\\", bucket.name);
  if (bela::PathExists(bucketdir)) {
    bela::fs::ForceDeleteFolders(bucketdir, ec);
  }
  if (MoveFileW(extractDir.data(), bucketdir.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

std::optional<baulk::Package> PackageMeta(std::wstring_view pkgMeta, std::wstring_view pkgName,
                                          std::wstring_view bucket, bela::error_code &ec) {
  auto pkj = baulk::json::parse_file(pkgMeta, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring{pkgName},
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
    pkg.hashValue = jv.fetch("url64.hash");
  } else if (jv.fetch_strings_checked("url", pkg.urls)) {
    pkg.hashValue = jv.fetch("url.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgMeta, L" not yet port to x64 platform.");
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
    pkg.hashValue = jv.fetch("urlarm64.hash");
  } else if (jv.fetch_strings_checked("url", pkg.urls)) {
    pkg.hashValue = jv.fetch("url.hash");
  } else if (jv.fetch_strings_checked("url64", pkg.urls)) {
    pkg.hashValue = jv.fetch("url64.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgMeta, L" not yet port to ARM64 platform.");
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
    pkg.hashValue = jv.fetch("url.hash");
  } else {
    ec = bela::make_error_code(bela::ErrGeneral, pkgMeta, L" not yet ported.");
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
std::optional<baulk::Package> PackageLocalMeta(std::wstring_view pkgName, bela::error_code &ec) {
  auto pkglock = bela::StringCat(baulk::vfs::AppLocks(), L"\\", pkgName, L".json");
  auto pkj = baulk::json::parse_file(pkglock, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring(pkgName),
      .version = jv.fetch("version"),
      .bucket = jv.fetch("bucket"),
  };
  jv.fetch_paths_checked("force_delete", pkg.forceDeletes);
  if (auto sv = jv.subview("venv"); sv) {
    pkg.venv.category = sv->fetch("category");
  }
  pkg.weights = baulk::BucketWeights(pkg.bucket);
  return std::make_optional(std::move(pkg));
}

bool PackageUpdatableMeta(const baulk::Package &pkgLocal, baulk::Package &pkg) {
  // initialize version from installed version
  bela::version pkgVersion(pkgLocal.version);
  auto weights = pkgLocal.weights;
  bool updated{false};
  auto bucketsDir = baulk::vfs::AppBuckets();
  for (const auto &bk : baulk::LoadedBuckets()) {
    auto pkgMeta = bela::StringCat(bucketsDir, L"\\", bk.name, L"\\bucket\\", pkgLocal.name, L".json");
    bela::error_code ec;
    auto pkgN = PackageMeta(pkgMeta, pkgLocal.name, bk.name, ec);
    if (!pkgN) {
      if (ec && ec.code != ENOENT) {
        bela::FPrintF(stderr, L"baulk: parse package meta error: %s\n", ec.message);
      }
      continue;
    }
    pkgN->bucket = bk.name;
    pkgN->weights = bk.weights;
    bela::version newVersion(pkgN->version);
    // compare version newVersion is > oldversion
    // newVersion == oldversion and strversion not equail compare weights
    if (newVersion > pkgVersion || (newVersion == pkgVersion && weights < pkgN->weights)) {
      pkg = std::move(*pkgN);
      pkgVersion = newVersion;
      weights = pkgN->weights;
      updated = true;
    }
  }
  return updated;
}
// package metadata
std::optional<baulk::Package> PackageMetaEx(std::wstring_view pkgName, bela::error_code &ec) {
  ec.clear();
  bela::version pkgVersion; // 0.0.0.0
  baulk::Package pkg;
  auto bucketsDir = baulk::vfs::AppBuckets();
  size_t pkgSame = 0;
  for (const auto &bk : baulk::LoadedBuckets()) {
    auto pkgMeta = bela::StringCat(bucketsDir, L"\\", bk.name, L"\\bucket\\", pkgName, L".json");
    auto pkgN = PackageMeta(pkgMeta, pkgName, bk.name, ec);
    if (!pkgN) {
      if (ec && ec.code != ENOENT) {
        bela::FPrintF(stderr, L"baulk: parse package meta error: %s\n", ec.message);
      }
      continue;
    }
    pkgSame++;
    pkgN->bucket = bk.name;
    pkgN->weights = bk.weights;
    bela::version newVersion(pkgN->version);
    // compare version newVersion is > oldversion
    // newVersion == oldversion and strversion not equail compare weights
    if (newVersion > pkgVersion || (newVersion == pkgVersion && pkg.weights < pkgN->weights)) {
      pkg = std::move(*pkgN);
      pkgVersion = newVersion;
    }
  }
  if (pkgSame == 0) {
    ec = bela::make_error_code(ErrPackageNotYetPorted, L"'", pkgName, L"' not yet ported.");
    return std::nullopt;
  }
  return std::make_optional(std::move(pkg));
}

bool PackageIsUpdatable(std::wstring_view pkgName, baulk::Package &pkg) {
  bela::error_code ec;
  auto localMeta = PackageLocalMeta(pkgName, ec);
  if (!localMeta) {
    return false;
  }
  return PackageUpdatableMeta(*localMeta, pkg);
}

} // namespace baulk::bucket
