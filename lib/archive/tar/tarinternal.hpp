//
#ifndef BAULK_ARCHIVE_TAR_INTERNAL_HPP
#define BAULK_ARCHIVE_TAR_INTERNAL_HPP
#include <baulk/archive/tar.hpp>
#include <baulk/allocate.hpp>
#include <baulk/archive.hpp>
#include <charconv>

namespace baulk::archive::tar {
using baulk::mem::Buffer;
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

constexpr std::string_view paxNone = ""; // Indicates that no PAX key is suitable
constexpr std::string_view paxPath = "path";
constexpr std::string_view paxLinkpath = "linkpath";
constexpr std::string_view paxSize = "size";
constexpr std::string_view paxUid = "uid";
constexpr std::string_view paxGid = "gid";
constexpr std::string_view paxUname = "uname";
constexpr std::string_view paxGname = "gname";
constexpr std::string_view paxMtime = "mtime";
constexpr std::string_view paxAtime = "atime";
constexpr std::string_view paxCtime = "ctime";     // Removed from later revision of PAX spec, but was valid
constexpr std::string_view paxCharset = "charset"; // Currently unused
constexpr std::string_view paxComment = "comment"; // Currently unused

constexpr std::string_view paxSchilyXattr = "SCHILY.xattr.";

// Keywords for GNU sparse files in a PAX extended header.
constexpr std::string_view paxGNUSparse = "GNU.sparse.";
constexpr std::string_view paxGNUSparseNumBlocks = "GNU.sparse.numblocks";
constexpr std::string_view paxGNUSparseOffset = "GNU.sparse.offset";
constexpr std::string_view paxGNUSparseNumBytes = "GNU.sparse.numbytes";
constexpr std::string_view paxGNUSparseMap = "GNU.sparse.map";
constexpr std::string_view paxGNUSparseName = "GNU.sparse.name";
constexpr std::string_view paxGNUSparseMajor = "GNU.sparse.major";
constexpr std::string_view paxGNUSparseMinor = "GNU.sparse.minor";
constexpr std::string_view paxGNUSparseSize = "GNU.sparse.size";
constexpr std::string_view paxGNUSparseRealSize = "GNU.sparse.realsize";
bool parsePAXTime(std::string_view p, bela::Time &t, bela::error_code &ec);
bool mergePAX(Header &h, pax_records_t &paxHdrs, bela::error_code &ec);
bool parsePAXRecord(std::string_view *sv, std::string_view *k, std::string_view *v, bela::error_code &ec);
bool validateSparseEntries(sparseDatas &spd, int64_t size);
tar_format_t getFormat(const ustar_header &hdr);
inline std::string parseString(const void *data, size_t N) {
  auto p = reinterpret_cast<const char *>(data);
  auto pos = memchr(p, 0, N);
  if (pos == nullptr) {
    return std::string(p, N);
  }
  N = reinterpret_cast<const char *>(pos) - p;
  return std::string(p, N);
}

template <size_t N> std::string parseString(const char (&aArr)[N]) { return parseString(aArr, N); }

inline int64_t parseNumeric8(const char *p, size_t char_cnt) {
  int64_t val = 0;
  auto res = std::from_chars(p, p + char_cnt, val, 8);
  return val;
}

inline int64_t parseNumeric10(const char *p, size_t char_cnt) {
  int64_t val = 0;
  auto res = std::from_chars(p, p + char_cnt, val, 10);
  return val;
}

/*
 * Parse a base-256 integer.  This is just a variable-length
 * twos-complement signed binary value in big-endian order, except
 * that the high-order bit is ignored.  The values here can be up to
 * 12 bytes, so we need to be careful about overflowing 64-bit
 * (8-byte) integers.
 *
 * This code unashamedly assumes that the local machine uses 8-bit
 * bytes and twos-complement arithmetic.
 */
int64_t parseNumeric256(const char *_p, size_t char_cnt);

/*-
 * Convert text->integer.
 *
 * Traditional tar formats (including POSIX) specify base-8 for
 * all of the standard numeric fields.  This is a significant limitation
 * in practice:
 *   = file size is limited to 8GB
 *   = rdevmajor and rdevminor are limited to 21 bits
 *   = uid/gid are limited to 21 bits
 *
 * There are two workarounds for this:
 *   = pax extended headers, which use variable-length string fields
 *   = GNU tar and STAR both allow either base-8 or base-256 in
 *      most fields.  The high bit is set to indicate base-256.
 *
 * On read, this implementation supports both extensions.
 */
inline int64_t parseNumeric(const char *p, size_t char_cnt) {
  /*
   * Technically, GNU tar considers a field to be in base-256
   * only if the first byte is 0xff or 0x80.
   */
  if ((*p & 0x80) != 0) {
    return (parseNumeric256(p, char_cnt));
  }
  return (parseNumeric8(p, char_cnt));
}

template <size_t N> int64_t parseNumeric(const char (&aArr)[N]) { return parseNumeric(aArr, N); }

} // namespace baulk::archive::tar

#endif