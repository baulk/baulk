//
#ifndef BAULK_ARCHIVE_TAR_INTERNAL_HPP
#define BAULK_ARCHIVE_TAR_INTERNAL_HPP
#include <tar.hpp>

namespace baulk::archive::tar {
// https://www.gnu.org/software/tar/manual/html_node/Formats.html
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

struct tar_v7_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
};
struct star_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char rdevmajor[8];
  char rdevminor[8];
  char prefix[131];
  char atime[12];
  char ctime[12];
  char trailer[4];
};

} // namespace baulk::archive::tar

#endif