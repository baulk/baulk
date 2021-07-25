///
#include "internal.hpp"

namespace bela::pe {
typedef enum ReplacesGeneralNumericDefines {
// Directory entry macro for CLR data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
  IMAGE_DIRECTORY_ENTRY_COMHEADER = 14,
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER
} ReplacesGeneralNumericDefines;

// https://github.com/dotnet/runtime/blob/main/src/coreclr/inc/mdfileformat.h
// https://github.com/dotnet/runtime/blob/main/src/coreclr/utilcode/pedecoder.cpp

//*****************************************************************************
// The signature ULONG is the first 4 bytes of the file format.  The second
// signature string starts the header containing the stream list.  It is used
// for an integrity check when reading the header in lieu of a more complicated
// system.
//*****************************************************************************
#define STORAGE_MAGIC_SIG 0x424A5342 // BSJB

//*****************************************************************************
// These values get written to the signature at the front of the file.  Changing
// these values should not be done lightly because all old files will no longer
// be supported.  In a future revision if a format change is required, a
// backwards compatible migration path must be provided.
//*****************************************************************************

#define FILE_VER_MAJOR 1
#define FILE_VER_MINOR 1

// These are the last legitimate 0.x version macros.  The file format has
// sinced move up to 1.x (see macros above).  After COM+ 1.0/NT 5 RTM's, these
// macros should no longer be required or ever seen.
#define FILE_VER_MAJOR_v0 0

#define FILE_VER_MINOR_v0 19

#define MAXSTREAMNAME 32

enum {
  STGHDR_NORMAL = 0x00,    // Normal default flags.
  STGHDR_EXTRADATA = 0x01, // Additional data exists after header.
};

#pragma pack(push, 1)
struct STORAGESIGNATURE {
  uint32_t lSignature;     // "Magic" signature.
  uint16_t iMajorVer;      // Major file version.
  uint16_t iMinorVer;      // Minor file version.
  uint32_t iExtraData;     // Offset to next structure of information
  uint32_t iVersionString; // Length of version string
  // uint8_t VersionString[1];
};
struct STORAGEHEADER {
  uint8_t fFlags; // STGHDR_xxx flags.
  uint8_t pad;
  uint16_t iStreams;
};
struct STORAGESTREAM {
  uint32_t iOffset;           // Offset in file for this stream.
  uint32_t iSize;             // Size of the file.
  char rcName[MAXSTREAMNAME]; // Start of name, null terminated.
};
#pragma pack(pop)

void FlagsToText(const IMAGE_COR20_HEADER *cr, std::string &text) {
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_ILONLY)) {
    text.append("IL only, ");
  }
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_32BITREQUIRED)) {
    text.append("32-bit only, ");
  }
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_32BITPREFERRED)) {
    text.append("32-bit preferred, ");
  }
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_IL_LIBRARY)) {
    text.append("IL library, ");
  }
  if (cr->StrongNameSignature.VirtualAddress != 0 && cr->StrongNameSignature.Size != 0) {
    if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_STRONGNAMESIGNED)) {
      text.append("Strong-name signed, ");
    } else {
      text.append("Strong-name delay signed, ");
    }
  }
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_NATIVE_ENTRYPOINT)) {
    text.append("Native entry-point, ");
  }
  if (bela::FlagIsTrue(cr->Flags, COMIMAGE_FLAGS_TRACKDEBUGDATA)) {
    text.append("Track debug data, ");
  }
  if (text.size() > 2) {
    text.resize(text.size() - 2);
  }
}

std::optional<DotNetMetadata> File::LookupDotNetMetadata(bela::error_code &ec) const {
  auto clrd = getDataDirectory(IMAGE_DIRECTORY_ENTRY_COMHEADER);
  if (clrd == nullptr) {
    return std::nullopt;
  }
  auto sec = getSection(clrd);
  if (sec == nullptr) {
    return std::nullopt;
  }
  auto sdata = readSectionData(*sec, ec);
  if (!sdata) {
    return std::nullopt;
  }
  auto bv = sdata->as_bytes_view();
  auto N = clrd->VirtualAddress - sec->VirtualAddress;
  auto cr = bv.checked_cast<IMAGE_COR20_HEADER>(N);
  if (cr == nullptr) {
    return std::nullopt;
  }
  auto va = bela::fromle(cr->MetaData.VirtualAddress);
  if (va < sec->VirtualAddress) {
    return std::nullopt;
  }
  if ((N = va - sec->VirtualAddress) >= sec->Size) {
    // avoid bad data
    return std::nullopt;
  }
  auto d = bv.checked_cast<STORAGESIGNATURE>(N);
  if (d == nullptr) {
    return std::nullopt;
  }
  if (bela::fromle(d->lSignature) != STORAGE_MAGIC_SIG) {
    return std::nullopt;
  }
  DotNetMetadata dm;
  FlagsToText(cr, dm.flags);
  dm.version = bv.make_cstring_view(N + sizeof(STORAGESIGNATURE));
  return std::make_optional(std::move(dm));
}

} // namespace bela::pe