///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>

namespace baulk::archive::zip {
bool DecompressFile(HANDLE &FileHandle, std::wstring_view dest,
                    bela::error_code &ec);
} // namespace baulk::archive::zip

#endif