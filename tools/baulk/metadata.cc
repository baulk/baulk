// load package metadata
#include <baulk/json_utils.hpp>
#include <baulk/vfs.hpp>
#include <bela/fnmatch.hpp>
#include <bela/ascii.hpp>
#include <baulk/fs.hpp>
#include "bucket.hpp"

namespace baulk {

inline auto PackageMetaJoinNative(const Bucket &bucket, std::wstring_view pkgName) {
  return bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name, L"\\bucket\\", pkgName, L".json");
}
#if defined(_M_X64)
constexpr std::string_view host_architecture = "64bit";
constexpr std::wstring_view host_architecture_name = L"x64";
#elif defined(_M_ARM64)
constexpr std::string_view host_architecture = "arm64";
constexpr std::wstring_view host_architecture_name = L"ARM64";
#else
constexpr std::string_view host_architecture = "32bit";
constexpr std::wstring_view host_architecture_name = L"x86";
#endif

constexpr std::string_view arm64_architecture = "arm64";
constexpr std::string_view x64bit_architecture = "64bit";
constexpr std::string_view x32bit_architecture = "32bit";

inline void PackageResolveURL(Package &pkg, baulk::json_view &jv) {
  using namespace std::string_view_literals;
  auto __ = bela::finally([&] {
    if (pkg.links.empty()) {
      jv.fetch_paths_checked("links", pkg.links);
    }
    if (pkg.launchers.empty()) {
      jv.fetch_paths_checked("launchers", pkg.launchers);
    }
    if (pkg.urls.empty()) {
      pkg.urls.emplace_back(jv.fetch("url"));
      pkg.hash = jv.fetch("hash");
    }
    if (pkg.hash.empty()) {
      pkg.hash = jv.fetch("url.hash");
    }
  });
  auto archLoad = [&](baulk::json_view &jv_, const std::string_view arch) -> bool {
    if (auto av = jv_.subview(arch); av) {
      pkg.urls.emplace_back(av->fetch("url"));
      pkg.hash = av->fetch("hash");
      jv.fetch_paths_checked("links", pkg.links);
      jv.fetch_paths_checked("launchers", pkg.launchers);
      return true;
    }
    return false;
  };
  if (auto sv = jv.subview("architecture"); sv) {
    if (archLoad(*sv, host_architecture)) {
      return;
    }
    if constexpr (host_architecture != x64bit_architecture) {
      if (archLoad(*sv, x64bit_architecture)) {
        return;
      }
    }
    if (archLoad(*sv, x32bit_architecture)) {
      return;
    }
  }
  if constexpr (host_architecture == x64bit_architecture) {
    jv.fetch_strings_checked("url64", pkg.urls);
    pkg.hash = jv.fetch("url64.hash");
    return;
  }
  if constexpr (host_architecture == arm64_architecture) {
    jv.fetch_strings_checked("urlarm64", pkg.urls);
    pkg.hash = jv.fetch("urlarm64.hash");
  }
}

std::optional<baulk::Package> PackageMetaNative(const Bucket &bucket, std::wstring_view pkgName, bela::error_code &ec) {
  auto pkgMeta = PackageMetaJoinNative(bucket, pkgName);
  auto pkj = baulk::parse_json_file(pkgMeta, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring{pkgName},
      .description = jv.fetch("description"),
      .version = jv.fetch("version"),
      .bucket = std::wstring{bucket.name},
      .extension = bela::AsciiStrToLower(jv.fetch("extension", L"auto")), // to lower
      .homepage = jv.fetch("homepage"),
      .notes = jv.fetch("notes"),
      .license = jv.fetch("license"),
  };
  jv.fetch_strings_checked("suggest", pkg.suggest);
  jv.fetch_paths_checked("force_delete", pkg.forceDeletes);
  PackageResolveURL(pkg, jv);
  if (pkg.urls.empty()) {
    ec = bela::make_error_code(bela::ErrGeneral, pkgMeta, L" not yet port to ", host_architecture_name, L" platform.");
    return std::nullopt;
  }
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

inline auto PackageMetaJoinScoop(const Bucket &bucket, std::wstring_view pkgName) {
  return bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name, L"\\bucket\\", pkgName, L".json");
}

// Not support multi file and ....
std::optional<baulk::Package> PackageMetaScoop(const Bucket &bucket, std::wstring_view pkgName, bela::error_code &ec) {
#if !defined(_M_X64) && !defined(_M_ARM64)
  return std::nullopt;
#endif
  auto pkgMeta = PackageMetaJoinNative(bucket, pkgName);
  auto pkj = baulk::parse_json_file(pkgMeta, ec);
  if (!pkj) {
    return std::nullopt;
  }
  auto jv = pkj->view();
  Package pkg{
      .name = std::wstring{pkgName},
      .description = jv.fetch("description"),
      .version = jv.fetch("version"),
      .bucket = std::wstring{bucket.name},
      .extension = L"auto", // always auto
      .homepage = jv.fetch("homepage"),
      .notes = jv.fetch("notes"),
      .license = jv.fetch("license"),
  };
  pkg.variant = BucketVariant::Scoop;
  jv.fetch_paths_checked("bin", pkg.launchers);

  if (auto sv = jv.subview("architecture"); sv) {
    if (auto av = sv->subview("64bit"); av) {
      pkg.urls.emplace_back(av->fetch("url"));
      pkg.hash = av->fetch("hash");
      return std::make_optional(std::move(pkg));
    }
  }
  pkg.urls.emplace_back(jv.fetch("url"));
  pkg.hash = jv.fetch("hash");
  return std::make_optional(std::move(pkg));
}

std::optional<baulk::Package> PackageMeta(const Bucket &bucket, std::wstring_view pkgName, bela::error_code &ec) {
  switch (bucket.variant) {
  case BucketVariant::Native:
    return PackageMetaNative(bucket, pkgName, ec);
  case BucketVariant::Scoop:
    return PackageMetaScoop(bucket, pkgName, ec);
  default:
    break;
  }
  ec = bela::make_error_code(bela::ErrUnimplemented, L"bucket not support variant: ", static_cast<int>(bucket.variant));
  return std::nullopt;
}

bool PackageMatchedInternal(const Bucket &bucket, std::wstring_view pkgMetaFolder, const OnPattern &op,
                            const OnMatched &om) {
  DbgPrint(L"search bucket: %s, metadata folder: %s", bucket.name, pkgMetaFolder);
  bela::fs::Finder finder;
  bela::error_code ec;
  if (!finder.First(pkgMetaFolder, L"*.json", ec)) {
    return false;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    auto pkgName = finder.Name();
    pkgName.remove_suffix(5);
    if (!op(pkgName)) {
      continue;
    }
    om(bucket, pkgName);
  } while (finder.Next());
  return true;
}

bool PackageMatched(const OnPattern &op, const OnMatched &om) {
  for (const auto &bucket : LoadedBuckets()) {
    switch (bucket.variant) {
    case BucketVariant::Native: {
      auto pkgMetaFolder = bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name, L"\\bucket\\");
      PackageMatchedInternal(bucket, pkgMetaFolder, op, om);
    } break;
    case BucketVariant::Scoop: {
      auto pkgMetaFolder = bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name, L"\\bucket\\");
      PackageMatchedInternal(bucket, pkgMetaFolder, op, om);
    } break;
    default:
      break;
    }
  }
  return true;
}

} // namespace baulk
