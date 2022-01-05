//
#ifndef BAULK_BUCKET_HPP
#define BAULK_BUCKET_HPP
#include <string>
#include <optional>
#include <functional>
#include <bela/base.hpp>
#include "baulk.hpp"

namespace baulk {
constexpr long ErrPackageNotYetPorted = bela::ErrUnimplemented + 1000;

std::optional<std::wstring> BucketNewest(const baulk::Bucket &bucket, bela::error_code &ec);
bool BucketUpdate(const baulk::Bucket &bucket, std::wstring_view id, bela::error_code &ec);
// PackageMeta from file
std::optional<baulk::Package> PackageMeta(const Bucket &bucket, std::wstring_view pkgName, bela::error_code &ec);

using OnPattern = std::function<bool(std::wstring_view pkgName)>;
using OnMatched = std::function<bool(const Bucket &bucket, std::wstring_view pkgName)>;
// PackageMatched search support
bool PackageMatched(const OnPattern &op, const OnMatched &om);

// PackageMeta from package name. search --
std::optional<baulk::Package> PackageMetaEx(std::wstring_view pkgname, bela::error_code &ec);
// installed package meta;
std::optional<baulk::Package> PackageLocalMeta(std::wstring_view pkgname, bela::error_code &ec);
bool PackageUpdatableMeta(const baulk::Package &opkg, baulk::Package &pkg);

bool PackageIsUpdatable(std::wstring_view pkgname, baulk::Package &pkg);
} // namespace baulk

#endif
