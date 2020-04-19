///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>

namespace baulk::archive::zip {
bool ZipExtract(std::wstring_view file, std::wstring_view dest,
                bela::error_code &ec);

} // namespace zip

#endif