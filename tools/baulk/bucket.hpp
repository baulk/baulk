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
                  bela::error_code &ec);
} // namespace baulk::bucket

#endif