///
#ifndef BAULK_LAUNCHER_HPP
#define BAULK_LAUNCHER_HPP
#include "baulk.hpp"

namespace baulk {
bool MakePackageLinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec);
bool RemovePackageLinks(std::wstring_view pkg, bela::error_code &ec);
} // namespace baulk

#endif