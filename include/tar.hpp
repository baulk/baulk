/// Tar
#ifndef BAULK_TAR_HPP
#define BAULK_TAR_HPP
#include <cstdint>
#include <bela/base.hpp>

namespace baulk::archive::tar {
// https://www.mkssoftware.com/docs/man4/tar.4.asp
/*  File Header (512 bytes)
 *  Offst Size Field
 *      Pre-POSIX Header
 *  0     100  File name
 *  100   8    File mode
 *  108   8    Owner's numeric user ID
 *  116   8    Group's numeric user ID
 *  124   12   File size in bytes (octal basis)
 *  136   12   Last modification time in numeric Unix time format (octal)
 *  148   8    Checksum for header record
 *  156   1    Type flag
 *  157   100  Name of linked file
 *      UStar Format
 *  257   6    UStar indicator "ustar"
 *  263   2    UStar version "00"
 *  265   32   Owner user name
 *  297   32   Owner group name
 *  329   8    Device major number
 *  337   8    Device minor number
 *  345   155  Filename prefix
 */
constexpr char magicGNU[] = {'u', 's', 't', 'a', 'r', ' '};
constexpr char versionGNU[] = {' ', 0x00};
constexpr char magicUSTAR[] = {'u', 's', 't', 'a', 'r', 0x00};
constexpr char versionUSTAR[] = {'0', '0'};
constexpr char trailerSTAR[] = {'t', 'a', 'r', 0x00};

constexpr size_t blockSize = 512;  // Size of each block in a tar stream
constexpr size_t nameSize = 100;   // Max length of the name field in USTAR format
constexpr size_t prefixSize = 155; // Max length of the prefix field in USTAR format

struct tar_header {   /* byte offset */
  char name[100];     /*   0 */
  char mode[8];       /* 100 */
  char uid[8];        /* 108 */
  char gid[8];        /* 116 */
  char size[12];      /* 124 */
  char mtime[12];     /* 136 */
  char chksum[8];     /* 148 */
  char typeflag;      /* 156 */
  char linkname[100]; /* 157 */
  char magic[6];      /* 257 */
  char version[2];    /* 263 */
  char uname[32];     /* 265 */
  char gname[32];     /* 297 */
  char devmajor[8];   /* 329 */
  char devminor[8];   /* 337 */
  char prefix[155];   /* 345 */
                      /* 500 */
};

class Reader {
public:
private:
};
} // namespace baulk::archive::tar

#endif