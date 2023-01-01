//
#ifndef BAULK_ZIP_INTERNAL_HPP
#define BAULK_ZIP_INTERNAL_HPP
#include <bela/path.hpp>
#include <baulk/archive/zip.hpp>
#include <baulk/allocate.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/crc32.hpp>

namespace baulk::archive::zip {
using baulk::mem::Buffer;
// https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
// https://en.wikipedia.org/wiki/ZIP_(file_format)
// https://en.wikipedia.org/wiki/Comparison_of_file_archivers
// https://en.wikipedia.org/wiki/List_of_archive_formats
constexpr int fileHeaderSignature = 0x04034b50;
constexpr int directoryHeaderSignature = 0x02014b50;
constexpr int directoryEndSignature = 0x06054b50;
constexpr int directory64LocSignature = 0x07064b50;
constexpr int directory64EndSignature = 0x06064b50;
constexpr uint32_t dataDescriptorSignature = 0x08074b50; // de-facto standard; required by OS X Finder
constexpr uint32_t fileHeaderLen = 30;                   // + filename + extra
constexpr int directoryHeaderLen = 46;                   // + filename + extra + comment
constexpr int directoryEndLen = 22;                      // + comment
constexpr int dataDescriptorLen = 16;   // four uint32: descriptor signature, crc32, compressed size, size
constexpr int dataDescriptor64Len = 24; // descriptor with 8 byte sizes
constexpr int directory64LocLen = 20;   //
constexpr int directory64EndLen = 56;   // + extra

// Constants for the first byte in CreatorVersion.
constexpr int creatorFAT = 0;
constexpr int creatorUnix = 3;
constexpr int creatorNTFS = 11;
constexpr int creatorVFAT = 14;
constexpr int creatorMacOSX = 19;

// Version numbers.
constexpr int zipVersion20 = 20; // 2.0
constexpr int zipVersion45 = 45; // 4.5 (reads and writes zip64 archives)

// Limits for non zip64 files.
constexpr auto uint16max = (std::numeric_limits<uint16_t>::max)();
constexpr auto uint32max = (std::numeric_limits<uint32_t>::max)();

// Extra header IDs.
//
// IDs 0..31 are reserved for official use by PKWARE.
// IDs above that range are defined by third-party vendors.
// Since ZIP lacked high precision timestamps (nor a official specification
// of the timezone used for the date fields), many competing extra fields
// have been invented. Pervasive use effectively makes them "official".
//
// See http://mdfs.net/Docs/Comp/Archiving/Zip/ExtraField

// 0x07c8        Macintosh
// 0x1986        Pixar USD header ID
// 0x2605        ZipIt Macintosh
// 0x2705        ZipIt Macintosh 1.3.5+
// 0x2805        ZipIt Macintosh 1.3.5+
// 0x334d        Info-ZIP Macintosh
// 0x4154        Tandem
// 0x4341        Acorn/SparkFS
// 0x4453        Windows NT security descriptor (binary ACL)
// 0x4704        VM/CMS
// 0x470f        MVS
// 0x4854        THEOS (old?)
// 0x4b46        FWKCS MD5 (see below)
// 0x4c41        OS/2 access control list (text ACL)
// 0x4d49        Info-ZIP OpenVMS
// 0x4d63        Macintosh Smartzip (??)
// 0x4f4c        Xceed original location extra field
// 0x5356        AOS/VS (ACL)
// 0x5455        extended timestamp
// 0x554e        Xceed unicode extra field
// 0x5855        Info-ZIP UNIX (original, also OS/2, NT, etc)
// 0x6375        Info-ZIP Unicode Comment Extra Field
// 0x6542        BeOS/BeBox
// 0x6854        THEOS
// 0x7075        Info-ZIP Unicode Path Extra Field
// 0x7441        AtheOS/Syllable
// 0x756e        ASi UNIX
// 0x7855        Info-ZIP UNIX (new)
// 0x7875        Info-ZIP UNIX (newer UID/GID)
// 0xa11e        Data Stream Alignment (Apache Commons-Compress)
// 0xa220        Microsoft Open Packaging Growth Hint
// 0xcafe        Java JAR file Extra Field Header ID
// 0xd935        Android ZIP Alignment Extra Field
// 0xe57a        Korean ZIP code page info
// 0xfd4a        SMS/QDOS
// 0x9901        AE-x encryption structure (see APPENDIX E)
// 0x9902        unknown

constexpr int zip64ExtraID = 0x0001;                     // Zip64 extended information
constexpr int ntfsExtraID = 0x000a;                      // NTFS
constexpr int unixExtraID = 0x000d;                      // UNIX
constexpr int extTimeExtraID = 0x5455;                   // Extended timestamp
constexpr int infoZipUnixExtraID = 0x5855;               // Info-ZIP Unix extension
constexpr int experimentalXlID = 0x6c78;                 // Experimental 'xl' field
constexpr int infoZipUnicodeCommentExtraID = 0x6375;     // Info-ZIP Unicode Comment Extra Field.
constexpr int infoZipUnicodePathID = 0x7075;             // Info-ZIP Unicode Path Extra Field.
constexpr int infoZipUnixExtra2ID = 0x7855;              // Info-ZIP Unix Extra Field (type 2) "Ux". *
constexpr int infoZipUnixExtra3ID = 0x7875;              // Info-Zip Unix Extra Field (type 3) "ux". *
constexpr int winzipAesExtraID = 0x9901;                 // winzip AES Extra Field
constexpr int msOpenPackagingGrowthHintExtraID = 0xa220; // Microsoft Open Packaging Growth Hint
constexpr int apkAlignExtraID = 0xD935;                  // Android ZIP Alignment Extra Field
constexpr int javaJARExtraID = 0xcafe;                   // Java JAR file Extra Field Header ID

constexpr auto s_IFMT = 0xf000;
constexpr auto s_IFSOCK = 0xc000;
constexpr auto s_IFLNK = 0xa000;
constexpr auto s_IFREG = 0x8000;
constexpr auto s_IFBLK = 0x6000;
constexpr auto s_IFDIR = 0x4000;
constexpr auto s_IFCHR = 0x2000;
constexpr auto s_IFIFO = 0x1000;
constexpr auto s_ISUID = 0x800;
constexpr auto s_ISGID = 0x400;
constexpr auto s_ISVTX = 0x200;

constexpr auto msdosDir = 0x10;
constexpr auto msdosReadOnly = 0x01;

inline bool IsSuperficialPath(std::string_view sv) {
  auto pv = bela::SplitPath(sv);
  return pv.size() <= 3;
}
constexpr size_t outsize = 64 * 1024;
constexpr size_t insize = 16 * 1024;
FileMode resolveFileMode(const File &file, uint32_t externalAttrs);
} // namespace baulk::archive::zip

#endif