// load package metadata
#include <baulk/json_utils.hpp>
#include <baulk/vfs.hpp>
#include <bela/fnmatch.hpp>
#include <bela/ascii.hpp>
#include <baulk/fs.hpp>
#include "bucket.hpp"

namespace baulk {

#if defined(_M_X64)
constexpr std::string_view host_architecture = "64bit";
constexpr std::wstring_view host_architecture_name = L"x64";
#elif defined(_M_ARM64)
constexpr std::string_view host_architecture = "arm64";
constexpr std::wstring_view host_architecture_name = L"ARM64";
#endif

constexpr std::string_view arm64_architecture = "arm64";
constexpr std::string_view x64bit_architecture = "64bit";
constexpr std::string_view x32bit_architecture = "32bit";

inline auto PackageMetaJoinNative(const Bucket &bucket, std::wstring_view pkgName) {
  return bela::StringCat(vfs::AppBuckets(), L"\\", bucket.name, L"\\bucket\\", pkgName, L".json");
}

inline void resolveNativeURL(Package &pkg, baulk::json_view &jv) {
  using namespace std::string_view_literals;
  auto _ = bela::finally([&] {
    if (pkg.links.empty()) {
      jv.get_paths_checked("links", pkg.links);
    }
    if (pkg.launchers.empty()) {
      jv.get_paths_checked("launchers", pkg.launchers);
    }
  });
  auto getArchURL = [&](baulk::json_view &jv_, const std::string_view arch) -> bool {
    if (auto av = jv_.subview(arch); av) {
      pkg.urls.emplace_back(av->get("url"));
      pkg.hash = av->get("hash");
      jv.get_paths_checked("links", pkg.links);
      jv.get_paths_checked("launchers", pkg.launchers);
      return true;
    }
    return false;
  };
  if (auto sv = jv.subview("architecture"); sv) {
    if (getArchURL(*sv, host_architecture)) {
      return;
    }
    if constexpr (host_architecture == arm64_architecture) {
      // ARM64
      if (getArchURL(*sv, x64bit_architecture)) {
        return;
      }
    }
    if (getArchURL(*sv, x32bit_architecture)) {
      return;
    }
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
      .description = jv.get("description"),
      .version = jv.get("version"),
      .bucket = std::wstring{bucket.name},
      .extension = bela::AsciiStrToLower(jv.get("extension", L"auto")), // to lower
      .homepage = jv.get("homepage"),
      .notes = jv.get("notes"),
      .license = jv.get("license"),
  };
  jv.get_strings_checked("suggest", pkg.suggest);
  jv.get_paths_checked("force_delete", pkg.forceDeletes);
  resolveNativeURL(pkg, jv);
  if (pkg.urls.empty()) {
    ec = bela::make_error_code(bela::ErrGeneral, pkgMeta, L" not yet port to ", host_architecture_name, L" platform.");
    return std::nullopt;
  }
  if (auto sv = jv.subview("venv"); sv) {
    DbgPrint(L"pkg '%s' support virtual env\n", pkg.name);
    pkg.venv.category = sv->get("category");
    sv->get_paths_checked("path", pkg.venv.paths);
    sv->get_paths_checked("include", pkg.venv.includes);
    sv->get_paths_checked("lib", pkg.venv.libs);
    sv->get_paths_checked("mkdir", pkg.venv.mkdirs);
    sv->get_strings_checked("env", pkg.venv.envs);
    sv->get_strings_checked("dependencies", pkg.venv.dependencies);
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
      .description = jv.get("description"),
      .version = jv.get("version"),
      .bucket = std::wstring{bucket.name},
      .extension = L"auto", // always auto
      .homepage = jv.get("homepage"),
      .notes = jv.get("notes"),
      .license = jv.get("license"),
  };
  pkg.variant = BucketVariant::Scoop;
  jv.get_paths_checked("bin", pkg.launchers);

  if (auto sv = jv.subview("architecture"); sv) {
    if (auto av = sv->subview(host_architecture); av) {
      pkg.urls.emplace_back(av->get("url"));
      pkg.hash = av->get("hash");
      return std::make_optional(std::move(pkg));
    }
    if (auto av = sv->subview(x32bit_architecture); av) {
      pkg.urls.emplace_back(av->get("url"));
      pkg.hash = av->get("hash");
      return std::make_optional(std::move(pkg));
    }
  }
  pkg.urls.emplace_back(jv.get("url"));
  pkg.hash = jv.get("hash");
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
