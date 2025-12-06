///
#ifndef BAULK_LAUNCHER_HPP
#define BAULK_LAUNCHER_HPP
#include "baulk.hpp"

namespace baulk {
bool NewLinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec);
bool DropLinks(std::wstring_view pkg, bela::error_code &ec);
} // namespace baulk

#endif