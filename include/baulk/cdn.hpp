///
#ifndef BAULK_CDN_HPP
#define BAULK_CDN_HPP
#include <string_view>

namespace baulk {
// https://baulk.io/cdn/hash/filename?url=u
constexpr std::wstring_view BaulkMirrorURL = L"X-Baulk-Mirror-URL";
constexpr std::wstring_view BaulkChecksumKey = L"X-Baulk-Mirror-Hash";
struct cdn {};
} // namespace baulk

#endif