//
#ifndef BAULK_BUCKET_HPP
#define BAULK_BUCKET_HPP
#include <string>
#include <optional>
#include <bela/base.hpp>
#include "baulk.hpp"

namespace baulk::bucket {
std::optional<std::wstring> BucketNewest(std::wstring_view bucketurl,
                                         bela::error_code &ec);
bool BucketUpdate(std::wstring_view bucketurl, std::wstring_view name,
                  std::wstring_view id, bela::error_code &ec);
// installed package meta;
std::optional<baulk::Package> PackageLocalMeta(std::wstring_view pkgname,
                                               bela::error_code &ec);
bool PackageUpdatableMeta(const baulk::Package &opkg, baulk::Package &pkg);
// package metadata
std::optional<baulk::Package> PackageMetaEx(std::wstring_view pkgname,
                                            bela::error_code &ec);
bool PackageIsUpdatable(std::wstring_view pkgname, baulk::Package &pkg);

} // namespace baulk::bucket

#endif
