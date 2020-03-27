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
bool PackageIsUpdatable(std::wstring_view pkgname, std::wstring_view path,
                        baulk::Package &pkg);

} // namespace baulk::bucket

#endif
