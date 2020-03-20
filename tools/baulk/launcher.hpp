///
#ifndef BAULK_LAUNCHER_HPP
#define BAULK_LAUNCHER_HPP
#include "baulk.hpp"

namespace baulk {
// baulk link and launcher
[[maybe_unused]] constexpr std::wstring_view BaulkLinkDir = L"bin\\linkbin";
[[maybe_unused]] constexpr std::wstring_view BaulkPkgsDir = L"bin\\pkgs";
[[maybe_unused]] constexpr std::wstring_view BaulkLinkMeta =
    L"bin\\linkbin\\baulk.linkmeta.json";

bool BaulkMakePkgLinks(const baulk::Package &pkg, bool forceoverwrite,
                  bela::error_code &ec);
bool BaulkRemovePkgLinks(std::wstring_view pkg, bela::error_code &ec);
} // namespace baulk

#endif