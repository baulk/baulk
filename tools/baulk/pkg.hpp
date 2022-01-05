//
#ifndef BAULK_PKG_HPP
#define BAULK_PKG_HPP
#include "baulk.hpp"

namespace baulk::package {
bool PackageInstall(const baulk::Package &pkg);
bool PackageForceDelete(std::wstring_view pkgname, bela::error_code &ec);
}; // namespace baulk::package

#endif