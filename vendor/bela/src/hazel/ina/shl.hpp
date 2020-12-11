////////////////
#ifndef HAZEL_SHELLLINK_HPP
#define HAZEL_SHELLLINK_HPP
#include <cstdint>

namespace shl {
// Shell link flags
// Thanks:
// https://github.com/reactos/reactos/blob/bfcbda227f99c1b59e8ed71f5e0f59f793d496a1/sdk/include/reactos/undocshell.h#L800
enum : uint32_t {
  SldfNone = 0x00000000,
  HasLinkTargetIDList = 0x00000001,
  HasLinkInfo = 0x00000002,
  HasName = 0x00000004,
  HasRelativePath = 0x00000008,
  HasWorkingDir = 0x00000010,
  HasArguments = 0x00000020,
  HasIconLocation = 0x00000040,
  IsUnicode = 0x00000080,
  ForceNoLinkInfo = 0x00000100,
  HasExpString = 0x00000200,
  RunInSeparateProcess = 0x00000400,
  Unused1 = 0x00000800,
  HasDrawinID = 0x00001000,
  RunAsUser = 0x00002000,
  HasExpIcon = 0x00004000,
  NoPidlAlias = 0x00008000,
  Unused2 = 0x00010000,
  RunWithShimLayer = 0x00020000,
  ForceNoLinkTrack = 0x00040000,
  EnableTargetMetadata = 0x00080000,
  DisableLinkPathTarcking = 0x00100000,
  DisableKnownFolderTarcking = 0x00200000,
  DisableKnownFolderAlia = 0x00400000,
  AllowLinkToLink = 0x00800000,
  UnaliasOnSave = 0x01000000,
  PreferEnvironmentPath = 0x02000000,
  KeepLocalIDListForUNCTarget = 0x04000000,
  PersistVolumeIDRelative = 0x08000000,
  SldfInvalid = 0x0ffff7ff,
  Reserved = 0x80000000
};

#pragma pack(1)
/*
SHELL_LINK = SHELL_LINK_HEADER [LINKTARGET_IDLIST] [LINKINFO]
 [STRING_DATA] *EXTRA_DATA
*/

struct filetime_t {
  uint32_t low;
  uint32_t high;
};

struct shell_link_t {
  uint32_t dwSize;
  uint8_t uuid[16];
  uint32_t linkflags;
  uint32_t fileattr;
  filetime_t createtime;
  filetime_t accesstime;
  filetime_t writetime;
  uint32_t filesize;
  uint32_t iconindex;
  uint32_t showcommand;
  uint16_t hotkey;
  uint16_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
};

enum link_info_flags : uint32_t {
  VolumeIDAndLocalBasePath = 0x00000001,
  CommonNetworkRelativeLinkAndPathSuffix = 0x00000002
};

struct shl_link_info_t {
  /* Size of the link info data */
  uint32_t cbSize;
  /* Size of this structure (ANSI: = 0x0000001C) */
  uint32_t cbHeaderSize;
  /* Specifies which fields are present/populated (SLI_*) */
  uint32_t dwFlags;
  /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
  uint32_t cbVolumeIDOffset;
  /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
  uint32_t cbLocalBasePathOffset;
  /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
  uint32_t cbCommonNetworkRelativeLinkOffset;
  /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
  uint32_t cbCommonPathSuffixOffset;
};

struct shl_link_infow_t {
  /* Size of the link info data */
  uint32_t cbSize;
  /* Size of this structure (Unicode: >= 0x00000024) */
  uint32_t cbHeaderSize;
  /* Specifies which fields are present/populated (SLI_*) */
  uint32_t dwFlags;
  /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
  uint32_t cbVolumeIDOffset;
  /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
  uint32_t cbLocalBasePathOffset;
  /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
  uint32_t cbCommonNetworkRelativeLinkOffset;
  /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
  uint32_t cbCommonPathSuffixOffset;
  /* Offset of the LocalBasePathUnicode field (Unicode, NULL-terminated string)
   */
  uint32_t cbLocalBasePathUnicodeOffset;
  /* Offset of the CommonPathSuffixUnicode field (Unicode, NULL-terminated
   * string) */
  uint32_t cbCommonPathSuffixUnicodeOffset;
};

// BlockSize (4 bytes): A 32-bit, unsigned integer that specifies the size of
// the KnownFolderDataBlock structure. This value MUST be 0x0000001C.

// BlockSignature (4 bytes): A 32-bit, unsigned integer that specifies the
// signature of the KnownFolderDataBlock extra data section. This value MUST be
// 0xA000000B.

// KnownFolderID (16 bytes): A value in GUID packet representation ([MS-DTYP]
// section 2.3.4.2) that specifies the folder GUID ID.

// Offset (4 bytes): A 32-bit, unsigned integer that specifies the location of
// the ItemID of the first child segment of the IDList specified by
// KnownFolderID. This value is the offset, in bytes, into the link target
// IDList.

struct shl_cnr_link_t {
  /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
  uint32_t cbSize;
  /* Specifies which fields are present/populated (SLI_CNR_*) */
  uint32_t dwFlags;
  /* Offset of the NetName field (ANSI, NULL–terminated string) */
  uint32_t cbNetNameOffset;
  /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
  uint32_t cbDeviceNameOffset;
  /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
  uint32_t dwNetworkProviderType;
};

struct shl_cnr_linkw_t {
  /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
  uint32_t cbSize;
  /* Specifies which fields are present/populated (SLI_CNR_*) */
  uint32_t dwFlags;
  /* Offset of the NetName field (ANSI, NULL–terminated string) */
  uint32_t cbNetNameOffset;
  /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
  uint32_t cbDeviceNameOffset;
  /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
  uint32_t dwNetworkProviderType;
  /* Offset of the NetNameUnicode field (Unicode, NULL–terminated string) */
  uint32_t cbNetNameUnicodeOffset;
  /* Offset of the DeviceNameUnicode field (Unicode, NULL–terminated string) */
  uint32_t cbDeviceNameUnicodeOffset;
};

struct knwonfolder_t {
  uint32_t blocksize; // LE 0x0000001C
  uint32_t blocksignature;
  uint8_t knownfolderid[16];
  uint32_t offset;
};

/*
 IDList ItemIDSize (2 bytes):
Data (variable):
 */
#pragma pack()

} // namespace shl

#endif
