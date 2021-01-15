//
#ifndef BAULK_ARCHIVE_TAR_INTERNAL_HPP
#define BAULK_ARCHIVE_TAR_INTERNAL_HPP
#include <tar.hpp>

namespace baulk::archive::tar {

constexpr char magicGNU[] = {'u', 's', 't', 'a', 'r', ' '};
constexpr char versionGNU[] = {' ', 0x00};
constexpr char magicUSTAR[] = {'u', 's', 't', 'a', 'r', 0x00};
constexpr char versionUSTAR[] = {'0', '0'};
constexpr char trailerSTAR[] = {'t', 'a', 'r', 0x00};

constexpr size_t blockSize = 512;  // Size of each block in a tar stream
constexpr size_t nameSize = 100;   // Max length of the name field in USTAR format
constexpr size_t prefixSize = 155; // Max length of the prefix field in USTAR format

constexpr size_t outsize = 64 * 1024;
constexpr size_t insize = 32 * 1024;


} // namespace baulk::archive::tar

#endif