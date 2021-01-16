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

} // namespace baulk::archive::tar

#endif