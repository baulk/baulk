//////////
#ifndef BELAWIN_REPARSEPOINT_INTERNAL_HPP
#define BELAWIN_REPARSEPOINT_INTERNAL_HPP
#pragma once
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif

// protocol
// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/c8e77b37-3909-4fe6-a4ea-2b9d423b1ee4
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT (0xA0000003L) // winnt
#endif

#ifndef IO_REPARSE_TAG_HSM
#define IO_REPARSE_TAG_HSM (0xC0000004L) // winnt
#endif

#ifndef IO_REPARSE_TAG_DRIVE_EXTENDER
#define IO_REPARSE_TAG_DRIVE_EXTENDER (0x80000005L)
#endif

#ifndef IO_REPARSE_TAG_HSM2
#define IO_REPARSE_TAG_HSM2 (0x80000006L) // winnt
#endif

#ifndef IO_REPARSE_TAG_SIS
#define IO_REPARSE_TAG_SIS (0x80000007L) // winnt
#endif

#ifndef IO_REPARSE_TAG_WIM
#define IO_REPARSE_TAG_WIM (0x80000008L) // winnt
#endif

#ifndef IO_REPARSE_TAG_CSV
#define IO_REPARSE_TAG_CSV (0x80000009L) // winnt
#endif

#ifndef IO_REPARSE_TAG_CSV
#define IO_REPARSE_TAG_CSV (0x8000000AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_FILTER_MANAGER
#define IO_REPARSE_TAG_FILTER_MANAGER (0x8000000BL)
#endif

#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK (0xA000000CL) // winnt
#endif

#ifndef IO_REPARSE_TAG_IIS_CACHE
#define IO_REPARSE_TAG_IIS_CACHE (0xA0000010L)
#endif

#ifndef IO_REPARSE_TAG_DFSR
#define IO_REPARSE_TAG_DFSR (0x80000012L) // winnt
#endif

#ifdef IO_REPARSE_TAG_DEDUP
#define IO_REPARSE_TAG_DEDUP (0x80000013L) // winnt
#endif

#ifndef IO_REPARSE_TAG_APPXSTRM
#define IO_REPARSE_TAG_APPXSTRM (0xC0000014L)
#endif

#ifndef IO_REPARSE_TAG_NFS
#define IO_REPARSE_TAG_NFS (0x80000014L) // winnt
#endif

#ifndef IO_REPARSE_TAG_NFS
#define IO_REPARSE_TAG_NFS (0x80000015L) // winnt
#endif

#ifndef IO_REPARSE_TAG_DFM
#define IO_REPARSE_TAG_DFM (0x80000016L)
#endif

#ifdef IO_REPARSE_TAG_WOF
#define IO_REPARSE_TAG_WOF (0x80000017L) // winnt
#endif

#ifndef IO_REPARSE_TAG_WOF
#define IO_REPARSE_TAG_WOF (0x80000018L) // winnt
#endif

#ifndef IO_REPARSE_TAG_WCI_1
#define IO_REPARSE_TAG_WCI_1 (0x90001018L) // winnt
#endif

#ifndef IO_REPARSE_TAG_GLOBAL_REPARSE
#define IO_REPARSE_TAG_GLOBAL_REPARSE (0xA0000019L) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD
#define IO_REPARSE_TAG_CLOUD (0x9000001AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_1
#define IO_REPARSE_TAG_CLOUD_1 (0x9000101AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_2
#define IO_REPARSE_TAG_CLOUD_2 (0x9000201AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_3
#define IO_REPARSE_TAG_CLOUD_3 (0x9000301AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_4
#define IO_REPARSE_TAG_CLOUD_4 (0x9000401AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_5
#define IO_REPARSE_TAG_CLOUD_5 (0x9000501AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_6
#define IO_REPARSE_TAG_CLOUD_6 (0x9000601AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_7
#define IO_REPARSE_TAG_CLOUD_7 (0x9000701AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_8
#define IO_REPARSE_TAG_CLOUD_8 (0x9000801AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_9
#define IO_REPARSE_TAG_CLOUD_9 (0x9000901AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_A
#define IO_REPARSE_TAG_CLOUD_A (0x9000A01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_B
#define IO_REPARSE_TAG_CLOUD_B (0x9000B01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_C
#define IO_REPARSE_TAG_CLOUD_C (0x9000C01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_D
#define IO_REPARSE_TAG_CLOUD_D (0x9000D01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_E
#define IO_REPARSE_TAG_CLOUD_E (0x9000E01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_F
#define IO_REPARSE_TAG_CLOUD_F (0x9000F01AL) // winnt
#endif

#ifndef IO_REPARSE_TAG_CLOUD_MASK
#define IO_REPARSE_TAG_CLOUD_MASK (0x0000F000L) // winnt
#endif

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK (0x8000001BL) // winnt
#endif

#ifndef IO_REPARSE_TAG_PROJFS
#define IO_REPARSE_TAG_PROJFS (0x9000001CL) // winnt
#endif

#ifndef IO_REPARSE_TAG_LX_SYMLINK
// Used by the Windows Subsystem for Linux (WSL) to represent a UNIX symbolic
// link. Server-side interpretation only, not meaningful over the wire.
#define IO_REPARSE_TAG_LX_SYMLINK (0xA000001DL) // Linux subsystem symbolic link
#endif

#ifndef IO_REPARSE_TAG_STORAGE_SYNC
#define IO_REPARSE_TAG_STORAGE_SYNC (0x8000001EL) // winnt
#endif

#ifndef IO_REPARSE_TAG_WCI_TOMBSTONE
#define IO_REPARSE_TAG_WCI_TOMBSTONE (0xA000001FL) // winnt
#endif

#ifndef IO_REPARSE_TAG_UNHANDLED
#define IO_REPARSE_TAG_UNHANDLED (0x80000020L) // winnt
#endif

#ifndef IO_REPARSE_TAG_ONEDRIVE
#define IO_REPARSE_TAG_ONEDRIVE (0x80000021L) // winnt
#endif

#ifndef IO_REPARSE_TAG_PROJFS_TOMBSTONE
// Used by the Windows Projected File System filter, for files managed by a user
// mode provider such as VFS for Git. Server-side interpretation only, not
// meaningful over the wire.
#define IO_REPARSE_TAG_PROJFS_TOMBSTONE (0xA0000022L) // winnt
#endif

#ifndef IO_REPARSE_TAG_AF_UNIX // AF_UNIX
// Used by the Windows Subsystem for Linux (WSL) to represent a UNIX domain
// socket. Server-side interpretation only, not meaningful over the wire.
#define IO_REPARSE_TAG_AF_UNIX (0x80000023L) // winnt
#endif

#ifndef IO_REPARSE_TAG_LX_FIFO
// Used by the Windows Subsystem for Linux (WSL) to represent a UNIX FIFO (named
// pipe). Server-side interpretation only, not meaningful over the wire.
#define IO_REPARSE_TAG_LX_FIFO (0x80000024L) // Linux subsystem FIFO
#endif

#ifndef IO_REPARSE_TAG_LX_CHR
// Used by the Windows Subsystem for Linux (WSL) to represent a UNIX character
// special file. Server-side interpretation only, not meaningful over the wire.
#define IO_REPARSE_TAG_LX_CHR (0x80000025L) // Linux  subsystem character device
#endif

#ifndef IO_REPARSE_TAG_LX_BLK
// Used by the Windows Subsystem for Linux (WSL) to represent a UNIX block
// special file. Server-side interpretation only, not meaningful over the wire.
#define IO_REPARSE_TAG_LX_BLK (0x80000026L) // Linux Subsystem block device
#endif

#ifndef IO_REPARSE_TAG_WCI_LINK
// Used by the Windows Container Isolation filter. Server-side interpretation
// only, not meaningful over the wire.
#define IO_REPARSE_TAG_WCI_LINK (0xA0000027L)
#endif

#ifndef IO_REPARSE_TAG_WCI_LINK_1
// Used by the Windows Container Isolation filter. Server-side interpretation
// only, not meaningful over the wire.
#define IO_REPARSE_TAG_WCI_LINK_1 (0xA0001027L)
#endif

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_reparse_data_buffer

#define SYMLINK_FLAG_RELATIVE                                                  \
  0x00000001 // If set then this is a relative symlink.
#define SYMLINK_DIRECTORY                                                      \
  0x80000000 // If set then this is a directory symlink. This is not persisted
             // on disk and is programmatically set by file system.
#define SYMLINK_FILE                                                           \
  0x40000000 // If set then this is a file symlink. This is not persisted on
             // disk and is programmatically set by file system.

#define SYMLINK_RESERVED_MASK                                                  \
  0xF0000000 // We reserve the high nibble for internal use

#if !defined(FSCTL_GET_REPARSE_POINT)
#define FSCTL_GET_REPARSE_POINT 0x900a8
#endif

#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1)
#define FSCTL_SET_REPARSE_POINT_EX                                             \
  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 259, METHOD_BUFFERED,                      \
           FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER_EX
// #define FSCTL_GET_REPARSE_POINT_EX                                             \
//   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 260, METHOD_BUFFERED,                      \
//            FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER_EX
#endif /* (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1) */

typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;         // Reparse tag type
  USHORT ReparseDataLength; // Length of the reparse data
  USHORT Reserved;          // Used internally by NTFS to store remaining length

  union {
    // Structure for IO_REPARSE_TAG_SYMLINK
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
      // Example of distinction between substitute and print names:
      // mklink /d ldrive c:\
      // SubstituteName: c:\\??\
      // PrintName: c:\

    } SymbolicLinkReparseBuffer;

    // Structure for IO_REPARSE_TAG_MOUNT_POINT
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;

    // Structure for IO_REPARSE_TAG_WIM
    // Handled by wimmount!FPOpenReparseTarget->wimserv.dll
    // (wimsrv!ImageExtract)
    struct {
      GUID ImageGuid;           // GUID of the mounted VIM image
      BYTE ImagePathHash[0x14]; // Hash of the path to the file within the image
    } WimImageReparseBuffer;

    // Structure for IO_REPARSE_TAG_WOF
    // Handled by FSCTL_GET_EXTERNAL_BACKING, FSCTL_SET_EXTERNAL_BACKING in NTFS
    // (Windows 10+)
    struct {
      //-- WOF_EXTERNAL_INFO --------------------
      ULONG Wof_Version;  // Should be 1 (WOF_CURRENT_VERSION)
      ULONG Wof_Provider; // Should be 2 (WOF_PROVIDER_FILE)

      //-- FILE_PROVIDER_EXTERNAL_INFO_V1 --------------------
      ULONG FileInfo_Version; // Should be 1 (FILE_PROVIDER_CURRENT_VERSION)
      ULONG
      FileInfo_Algorithm; // Usually 0 (FILE_PROVIDER_COMPRESSION_XPRESS4K)
    } WofReparseBuffer;

    // Structure for IO_REPARSE_TAG_APPEXECLINK
    struct {
      ULONG StringCount;   // Number of the strings in the StringList, separated
                           // by '\0'
      WCHAR StringList[1]; // Multistring (strings separated by '\0', terminated
                           // by '\0\0')
    } AppExecLinkReparseBuffer;

    // Structure for IO_REPARSE_TAG_WCI (0x80000018)
    struct {
      ULONG Version; // Expected to be 1 by wcifs.sys
      ULONG Reserved;
      GUID LookupGuid;      // GUID used for lookup in wcifs!WcLookupLayer
      USHORT WciNameLength; // Length of the WCI subname, in bytes
      WCHAR WciName[1];     // The WCI subname (not zero terminated)
    } WcifsReparseBuffer;

    // Handled by cldflt.sys!HsmpRpReadBuffer
    struct {
      USHORT Flags;    // Flags (0x8000 = not compressed)
      USHORT Length;   // Length of the data (uncompressed)
      BYTE RawData[1]; // To be RtlDecompressBuffer-ed
    } HsmReparseBufferRaw;

    // Dummy structure
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

typedef struct _REPARSE_DATA_BUFFER_EX {

  ULONG Flags;

  //
  //  This is the existing reparse tag on the file if any,  if the
  //  caller wants to replace the reparse tag too.
  //
  //    - To set the reparse data  along with the reparse tag that
  //      could be different,  pass the current reparse tag of the
  //      file.
  //
  //    - To update the reparse data while having the same reparse
  //      tag,  the caller should give the existing reparse tag in
  //      this ExistingReparseTag field.
  //
  //    - To set the reparse tag along with reparse data on a file
  //      that doesn't have a reparse tag yet, set this to zero.
  //
  //  If the ExistingReparseTag  does not match the reparse tag on
  //  the file,  the FSCTL_SET_REPARSE_POINT_EX  would  fail  with
  //  STATUS_IO_REPARSE_TAG_MISMATCH. NOTE: If a file doesn't have
  //  a reparse tag, ExistingReparseTag should be 0.
  //

  ULONG ExistingReparseTag;

  //
  //  For non-Microsoft reparse tags, this is the existing reparse
  //  guid on the file if any,  if the caller wants to replace the
  //  reparse tag and / or guid along with the data.
  //
  //  If ExistingReparseTag is 0, the file is not expected to have
  //  any reparse tags, so ExistingReparseGuid is ignored. And for
  //  non-Microsoft tags ExistingReparseGuid should match the guid
  //  in the file if ExistingReparseTag is non zero.
  //

  GUID ExistingReparseGuid;

  //
  //  Reserved
  //

  ULONGLONG Reserved;

  //
  //  Reparse data to set
  //

  union {

    REPARSE_DATA_BUFFER ReparseDataBuffer;
    REPARSE_GUID_DATA_BUFFER ReparseGuidDataBuffer;

  } DUMMYUNIONNAME;

} REPARSE_DATA_BUFFER_EX, *PREPARSE_DATA_BUFFER_EX;

#define REPARSE_DATA_BUFFER_HEADER_SIZE                                        \
  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

#endif
