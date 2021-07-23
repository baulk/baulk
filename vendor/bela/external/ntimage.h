/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntimage.h

Abstract:

    This is the include file that describes all image structures.

--*/

#ifndef _NTIMAGE_
#define _NTIMAGE_

#if _MSC_VER > 1000
#pragma once
#endif

#if _MSC_VER >= 1200
#pragma warning(push)
#pragma warning(disable:4820) /* padding added after data member */
#endif
#pragma warning (disable:4201) /* nonstandard extension used : nameless struct/union */
#pragma warning (disable:4214) /* nonstandard extension used : bit field types other then int */

//
// Define the linker version number.  This is temporary to aid
// in debugging with people trying to load images built with
// an older linker.  This is not required in the final product.
//

#define IMAGE_MAJOR_LINKER_VERSION 2

// begin_winnt

//
// Image Format
//

#ifndef _MAC

#include "pshpack4.h"                   // 4 byte packing is the default

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#include "pshpack2.h"                   // 16 bit headers are 2 byte packed

#else

#include "pshpack1.h"

#define IMAGE_DOS_SIGNATURE                 0x4D5A      // MZ
#define IMAGE_OS2_SIGNATURE                 0x4E45      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x4C45      // LE
#define IMAGE_NT_SIGNATURE                  0x50450000  // PE00
#endif

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    USHORT e_magic;                     // Magic number
    USHORT e_cblp;                      // Bytes on last page of file
    USHORT e_cp;                        // Pages in file
    USHORT e_crlc;                      // Relocations
    USHORT e_cparhdr;                   // Size of header in paragraphs
    USHORT e_minalloc;                  // Minimum extra paragraphs needed
    USHORT e_maxalloc;                  // Maximum extra paragraphs needed
    USHORT e_ss;                        // Initial (relative) SS value
    USHORT e_sp;                        // Initial SP value
    USHORT e_csum;                      // Checksum
    USHORT e_ip;                        // Initial IP value
    USHORT e_cs;                        // Initial (relative) CS value
    USHORT e_lfarlc;                    // File address of relocation table
    USHORT e_ovno;                      // Overlay number
    USHORT e_res[4];                    // Reserved words
    USHORT e_oemid;                     // OEM identifier (for e_oeminfo)
    USHORT e_oeminfo;                   // OEM information; e_oemid specific
    USHORT e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_OS2_HEADER {      // OS/2 .EXE header
    USHORT ne_magic;                    // Magic number
    CHAR   ne_ver;                      // Version number
    CHAR   ne_rev;                      // Revision number
    USHORT ne_enttab;                   // Offset of Entry Table
    USHORT ne_cbenttab;                 // Number of bytes in Entry Table
    LONG   ne_crc;                      // Checksum of whole file
    USHORT ne_flags;                    // Flag word
    USHORT ne_autodata;                 // Automatic data segment number
    USHORT ne_heap;                     // Initial heap allocation
    USHORT ne_stack;                    // Initial stack allocation
    LONG   ne_csip;                     // Initial CS:IP setting
    LONG   ne_sssp;                     // Initial SS:SP setting
    USHORT ne_cseg;                     // Count of file segments
    USHORT ne_cmod;                     // Entries in Module Reference Table
    USHORT ne_cbnrestab;                // Size of non-resident name table
    USHORT ne_segtab;                   // Offset of Segment Table
    USHORT ne_rsrctab;                  // Offset of Resource Table
    USHORT ne_restab;                   // Offset of resident name table
    USHORT ne_modtab;                   // Offset of Module Reference Table
    USHORT ne_imptab;                   // Offset of Imported Names Table
    LONG   ne_nrestab;                  // Offset of Non-resident Names Table
    USHORT ne_cmovent;                  // Count of movable entries
    USHORT ne_align;                    // Segment alignment shift count
    USHORT ne_cres;                     // Count of resource segments
    UCHAR  ne_exetyp;                   // Target Operating system
    UCHAR  ne_flagsothers;              // Other .EXE flags
    USHORT ne_pretthunks;               // offset to return thunks
    USHORT ne_psegrefbytes;             // offset to segment ref. bytes
    USHORT ne_swaparea;                 // Minimum code swap area size
    USHORT ne_expver;                   // Expected Windows version number
  } IMAGE_OS2_HEADER, *PIMAGE_OS2_HEADER;

typedef struct _IMAGE_VXD_HEADER {      // Windows VXD header
    USHORT e32_magic;                   // Magic number
    UCHAR  e32_border;                  // The byte ordering for the VXD
    UCHAR  e32_worder;                  // The word ordering for the VXD
    ULONG  e32_level;                   // The EXE format level for now = 0
    USHORT e32_cpu;                     // The CPU type
    USHORT e32_os;                      // The OS type
    ULONG  e32_ver;                     // Module version
    ULONG  e32_mflags;                  // Module flags
    ULONG  e32_mpages;                  // Module # pages
    ULONG  e32_startobj;                // Object # for instruction pointer
    ULONG  e32_eip;                     // Extended instruction pointer
    ULONG  e32_stackobj;                // Object # for stack pointer
    ULONG  e32_esp;                     // Extended stack pointer
    ULONG  e32_pagesize;                // VXD page size
    ULONG  e32_lastpagesize;            // Last page size in VXD
    ULONG  e32_fixupsize;               // Fixup section size
    ULONG  e32_fixupsum;                // Fixup section checksum
    ULONG  e32_ldrsize;                 // Loader section size
    ULONG  e32_ldrsum;                  // Loader section checksum
    ULONG  e32_objtab;                  // Object table offset
    ULONG  e32_objcnt;                  // Number of objects in module
    ULONG  e32_objmap;                  // Object page map offset
    ULONG  e32_itermap;                 // Object iterated data map offset
    ULONG  e32_rsrctab;                 // Offset of Resource Table
    ULONG  e32_rsrccnt;                 // Number of resource entries
    ULONG  e32_restab;                  // Offset of resident name table
    ULONG  e32_enttab;                  // Offset of Entry Table
    ULONG  e32_dirtab;                  // Offset of Module Directive Table
    ULONG  e32_dircnt;                  // Number of module directives
    ULONG  e32_fpagetab;                // Offset of Fixup Page Table
    ULONG  e32_frectab;                 // Offset of Fixup Record Table
    ULONG  e32_impmod;                  // Offset of Import Module Name Table
    ULONG  e32_impmodcnt;               // Number of entries in Import Module Name Table
    ULONG  e32_impproc;                 // Offset of Import Procedure Name Table
    ULONG  e32_pagesum;                 // Offset of Per-Page Checksum Table
    ULONG  e32_datapage;                // Offset of Enumerated Data Pages
    ULONG  e32_preload;                 // Number of preload pages
    ULONG  e32_nrestab;                 // Offset of Non-resident Names Table
    ULONG  e32_cbnrestab;               // Size of Non-resident Name Table
    ULONG  e32_nressum;                 // Non-resident Name Table Checksum
    ULONG  e32_autodata;                // Object # for automatic data object
    ULONG  e32_debuginfo;               // Offset of the debugging information
    ULONG  e32_debuglen;                // The length of the debugging info. in bytes
    ULONG  e32_instpreload;             // Number of instance pages in preload section of VXD file
    ULONG  e32_instdemand;              // Number of instance pages in demand load section of VXD file
    ULONG  e32_heapsize;                // Size of heap - for 16-bit apps
    UCHAR  e32_res3[12];                // Reserved words
    ULONG  e32_winresoff;
    ULONG  e32_winreslen;
    USHORT e32_devid;                   // Device ID for VxD
    USHORT e32_ddkver;                  // DDK version for VxD
  } IMAGE_VXD_HEADER, *PIMAGE_VXD_HEADER;

#ifndef _MAC
#include "poppack.h"                    // Back to 4 byte packing
#endif

//
// File header format.
//

typedef struct _IMAGE_FILE_HEADER {
    USHORT  Machine;
    USHORT  NumberOfSections;
    ULONG   TimeDateStamp;
    ULONG   PointerToSymbolTable;
    ULONG   NumberOfSymbols;
    USHORT  SizeOfOptionalHeader;
    USHORT  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER             20

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved external references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Aggressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_TARGET_HOST       0x0001  // Useful for indicating we want to interact with the host and not a WoW guest.
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2  // ARM Thumb/Thumb-2 Little-Endian
#define IMAGE_FILE_MACHINE_ARMNT             0x01c4  // ARM Thumb-2 Little-Endian
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
#define IMAGE_FILE_MACHINE_ARM64             0xAA64  // ARM64 Little-Endian
#define IMAGE_FILE_MACHINE_CEE               0xC0EE

// end_winnt

#define IMAGE_FILE_MACHINE_CHPE_X86          0x3A64
#define IMAGE_FILE_MACHINE_ARM64EC           0xA641
#define IMAGE_FILE_MACHINE_ARM64X            0xA64E

// begin_winnt

//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY {
    ULONG   VirtualAddress;
    ULONG   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    USHORT  Magic;
    UCHAR   MajorLinkerVersion;
    UCHAR   MinorLinkerVersion;
    ULONG   SizeOfCode;
    ULONG   SizeOfInitializedData;
    ULONG   SizeOfUninitializedData;
    ULONG   AddressOfEntryPoint;
    ULONG   BaseOfCode;
    ULONG   BaseOfData;

    //
    // NT additional fields.
    //

    ULONG   ImageBase;
    ULONG   SectionAlignment;
    ULONG   FileAlignment;
    USHORT  MajorOperatingSystemVersion;
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;
    USHORT  MinorSubsystemVersion;
    ULONG   Win32VersionValue;
    ULONG   SizeOfImage;
    ULONG   SizeOfHeaders;
    ULONG   CheckSum;
    USHORT  Subsystem;
    USHORT  DllCharacteristics;
    ULONG   SizeOfStackReserve;
    ULONG   SizeOfStackCommit;
    ULONG   SizeOfHeapReserve;
    ULONG   SizeOfHeapCommit;
    ULONG   LoaderFlags;
    ULONG   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_ROM_OPTIONAL_HEADER {
    USHORT Magic;
    UCHAR  MajorLinkerVersion;
    UCHAR  MinorLinkerVersion;
    ULONG  SizeOfCode;
    ULONG  SizeOfInitializedData;
    ULONG  SizeOfUninitializedData;
    ULONG  AddressOfEntryPoint;
    ULONG  BaseOfCode;
    ULONG  BaseOfData;
    ULONG  BaseOfBss;
    ULONG  GprMask;
    ULONG  CprMask[4];
    ULONG  GpValue;
} IMAGE_ROM_OPTIONAL_HEADER, *PIMAGE_ROM_OPTIONAL_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    USHORT      Magic;
    UCHAR       MajorLinkerVersion;
    UCHAR       MinorLinkerVersion;
    ULONG       SizeOfCode;
    ULONG       SizeOfInitializedData;
    ULONG       SizeOfUninitializedData;
    ULONG       AddressOfEntryPoint;
    ULONG       BaseOfCode;
    ULONGLONG   ImageBase;
    ULONG       SectionAlignment;
    ULONG       FileAlignment;
    USHORT      MajorOperatingSystemVersion;
    USHORT      MinorOperatingSystemVersion;
    USHORT      MajorImageVersion;
    USHORT      MinorImageVersion;
    USHORT      MajorSubsystemVersion;
    USHORT      MinorSubsystemVersion;
    ULONG       Win32VersionValue;
    ULONG       SizeOfImage;
    ULONG       SizeOfHeaders;
    ULONG       CheckSum;
    USHORT      Subsystem;
    USHORT      DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    ULONG       LoaderFlags;
    ULONG       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107

#ifdef _WIN64
typedef IMAGE_OPTIONAL_HEADER64             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR64_MAGIC
#else
typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR32_MAGIC
#endif

typedef struct _IMAGE_NT_HEADERS64 {
    ULONG Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS {
    ULONG Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_ROM_HEADERS {
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_ROM_OPTIONAL_HEADER OptionalHeader;
} IMAGE_ROM_HEADERS, *PIMAGE_ROM_HEADERS;

#ifdef _WIN64
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
#endif

// IMAGE_FIRST_SECTION doesn't need 32/64 versions since the file header is the same either way.

#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((ULONG_PTR)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))

// Subsystem Values

#define IMAGE_SUBSYSTEM_UNKNOWN              0   // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE               1   // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI          2   // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI          3   // Image runs in the Windows character subsystem.
// end_winnt
// reserved                                  4   // Old Windows CE subsystem.
// begin_winnt
#define IMAGE_SUBSYSTEM_OS2_CUI              5   // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI            7   // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS       8   // image is a native Win9x driver.
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       9   // Image runs in the Windows CE subsystem.
#define IMAGE_SUBSYSTEM_EFI_APPLICATION      10  //
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11   //
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER   12  //
#define IMAGE_SUBSYSTEM_EFI_ROM              13
#define IMAGE_SUBSYSTEM_XBOX                 14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16
#define IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG    17

// DllCharacteristics Entries

//      IMAGE_LIBRARY_PROCESS_INIT            0x0001     // Reserved.
//      IMAGE_LIBRARY_PROCESS_TERM            0x0002     // Reserved.
//      IMAGE_LIBRARY_THREAD_INIT             0x0004     // Reserved.
//      IMAGE_LIBRARY_THREAD_TERM             0x0008     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA    0x0020  // Image can handle a high entropy 64-bit virtual address space.
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040     // DLL can move.
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY    0x0080     // Code Integrity Image
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT    0x0100     // Image is NX compatible
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION 0x0200     // Image understands isolation and doesn't want it
#define IMAGE_DLLCHARACTERISTICS_NO_SEH       0x0400     // Image does not use SEH.  No SE handler may reside in this image
#define IMAGE_DLLCHARACTERISTICS_NO_BIND      0x0800     // Do not bind this image.
#define IMAGE_DLLCHARACTERISTICS_APPCONTAINER 0x1000     // Image should execute in an AppContainer
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER   0x2000     // Driver uses WDM model
#define IMAGE_DLLCHARACTERISTICS_GUARD_CF     0x4000     // Image supports Control Flow Guard.
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE     0x8000
// end_winnt
// Note: The Borland linker sets IMAGE_LIBRARY_xxx flags in DllCharacteristics

// LoaderFlags Values

#define IMAGE_LOADER_FLAGS_COMPLUS             0x00000001   // COM+ image
#define IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL       0x01000000   // Global subsections apply across TS sessions.

// begin_winnt

// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

//
// Non-COFF Object file header
//

typedef struct ANON_OBJECT_HEADER {
    USHORT  Sig1;            // Must be IMAGE_FILE_MACHINE_UNKNOWN
    USHORT  Sig2;            // Must be 0xffff
    USHORT  Version;         // >= 1 (implies the CLSID field is present)
    USHORT  Machine;
    ULONG   TimeDateStamp;
    CLSID   ClassID;         // Used to invoke CoCreateInstance
    ULONG   SizeOfData;      // Size of data that follows the header
} ANON_OBJECT_HEADER;

typedef struct ANON_OBJECT_HEADER_V2 {
    USHORT  Sig1;            // Must be IMAGE_FILE_MACHINE_UNKNOWN
    USHORT  Sig2;            // Must be 0xffff
    USHORT  Version;         // >= 2 (implies the Flags field is present - otherwise V1)
    USHORT  Machine;
    ULONG   TimeDateStamp;
    CLSID   ClassID;         // Used to invoke CoCreateInstance
    ULONG   SizeOfData;      // Size of data that follows the header
    ULONG   Flags;           // 0x1 -> contains metadata
    ULONG   MetaDataSize;    // Size of CLR metadata
    ULONG   MetaDataOffset;  // Offset of CLR metadata
} ANON_OBJECT_HEADER_V2;

typedef struct ANON_OBJECT_HEADER_BIGOBJ {
   /* same as ANON_OBJECT_HEADER_V2 */
    USHORT  Sig1;            // Must be IMAGE_FILE_MACHINE_UNKNOWN
    USHORT  Sig2;            // Must be 0xffff
    USHORT  Version;         // >= 2 (implies the Flags field is present)
    USHORT  Machine;         // Actual machine - IMAGE_FILE_MACHINE_xxx
    ULONG   TimeDateStamp;
    CLSID   ClassID;         // {D1BAA1C7-BAEE-4ba9-AF20-FAF66AA4DCB8}
    ULONG   SizeOfData;      // Size of data that follows the header
    ULONG   Flags;           // 0x1 -> contains metadata
    ULONG   MetaDataSize;    // Size of CLR metadata
    ULONG   MetaDataOffset;  // Offset of CLR metadata

    /* bigobj specifics */
    ULONG   NumberOfSections; // extended from WORD
    ULONG   PointerToSymbolTable;
    ULONG   NumberOfSymbols;
} ANON_OBJECT_HEADER_BIGOBJ;

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    UCHAR   Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            ULONG   PhysicalAddress;
            ULONG   VirtualSize;
    } Misc;
    ULONG   VirtualAddress;
    ULONG   SizeOfRawData;
    ULONG   PointerToRawData;
    ULONG   PointerToRelocations;
    ULONG   PointerToLinenumbers;
    USHORT  NumberOfRelocations;
    USHORT  NumberOfLinenumbers;
    ULONG   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

//
// Section characteristics.
//
//      IMAGE_SCN_TYPE_REG                   0x00000000  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
//      IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000
#define IMAGE_SCN_ALIGN_MASK                 0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

//
// TLS Characteristic Flags
//
#define IMAGE_SCN_SCALE_INDEX                0x00000001  // Tls index is scaled

#ifndef _MAC
#include "pshpack2.h"                       // Symbols, relocs, and linenumbers are 2 byte packed
#endif

//
// Symbol format.
//

typedef struct _IMAGE_SYMBOL {
    union {
        UCHAR   ShortName[8];
        struct {
            ULONG   Short;     // if 0, use LongName
            ULONG   Long;      // offset into string table
        } Name;
        ULONG   LongName[2];    // PUCHAR[2]
    } N;
    ULONG   Value;
    SHORT   SectionNumber;
    USHORT  Type;
    UCHAR   StorageClass;
    UCHAR   NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef IMAGE_SYMBOL UNALIGNED *PIMAGE_SYMBOL;

#define IMAGE_SIZEOF_SYMBOL                  18

typedef struct _IMAGE_SYMBOL_EX {
    union {
        UCHAR    ShortName[8];
        struct {
            ULONG   Short;     // if 0, use LongName
            ULONG   Long;      // offset into string table
        } Name;
        ULONG   LongName[2];    // PUCHAR [2]
    } N;
    ULONG   Value;
    LONG    SectionNumber;
    USHORT  Type;
    UCHAR   StorageClass;
    UCHAR   NumberOfAuxSymbols;
} IMAGE_SYMBOL_EX;
typedef IMAGE_SYMBOL_EX UNALIGNED *PIMAGE_SYMBOL_EX;

//
// Section values.
//
// Symbols have a section number of the section in which they are
// defined. Otherwise, section numbers have the following meanings:
//

#define IMAGE_SYM_UNDEFINED           (SHORT)0          // Symbol is undefined or is common.
#define IMAGE_SYM_ABSOLUTE            (SHORT)-1         // Symbol is an absolute value.
#define IMAGE_SYM_DEBUG               (SHORT)-2         // Symbol is a special debug item.
#define IMAGE_SYM_SECTION_MAX         0xFEFF            // Values 0xFF00-0xFFFF are special
#define IMAGE_SYM_SECTION_MAX_EX      MAXLONG

//
// Type (fundamental) values.
//

#define IMAGE_SYM_TYPE_NULL                 0x0000  // no type.
#define IMAGE_SYM_TYPE_VOID                 0x0001  //
#define IMAGE_SYM_TYPE_CHAR                 0x0002  // type character.
#define IMAGE_SYM_TYPE_SHORT                0x0003  // type short integer.
#define IMAGE_SYM_TYPE_INT                  0x0004  //
#define IMAGE_SYM_TYPE_LONG                 0x0005  //
#define IMAGE_SYM_TYPE_FLOAT                0x0006  //
#define IMAGE_SYM_TYPE_DOUBLE               0x0007  //
#define IMAGE_SYM_TYPE_STRUCT               0x0008  //
#define IMAGE_SYM_TYPE_UNION                0x0009  //
#define IMAGE_SYM_TYPE_ENUM                 0x000A  // enumeration.
#define IMAGE_SYM_TYPE_MOE                  0x000B  // member of enumeration.
#define IMAGE_SYM_TYPE_UCHAR                0x000C  //
#define IMAGE_SYM_TYPE_USHORT               0x000D  //
#define IMAGE_SYM_TYPE_UINT                 0x000E  //
#define IMAGE_SYM_TYPE_ULONG                0x000F  //
#define IMAGE_SYM_TYPE_PCODE                0x8000  //
//
// Type (derived) values.
//

#define IMAGE_SYM_DTYPE_NULL                0       // no derived type.
#define IMAGE_SYM_DTYPE_POINTER             1       // pointer.
#define IMAGE_SYM_DTYPE_FUNCTION            2       // function.
#define IMAGE_SYM_DTYPE_ARRAY               3       // array.

//
// Storage classes.
//
#define IMAGE_SYM_CLASS_END_OF_FUNCTION     (UCHAR)-1
#define IMAGE_SYM_CLASS_NULL                0x0000
#define IMAGE_SYM_CLASS_AUTOMATIC           0x0001
#define IMAGE_SYM_CLASS_EXTERNAL            0x0002
#define IMAGE_SYM_CLASS_STATIC              0x0003
#define IMAGE_SYM_CLASS_REGISTER            0x0004
#define IMAGE_SYM_CLASS_EXTERNAL_DEF        0x0005
#define IMAGE_SYM_CLASS_LABEL               0x0006
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL     0x0007
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT    0x0008
#define IMAGE_SYM_CLASS_ARGUMENT            0x0009
#define IMAGE_SYM_CLASS_STRUCT_TAG          0x000A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION     0x000B
#define IMAGE_SYM_CLASS_UNION_TAG           0x000C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION     0x000D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC    0x000E
#define IMAGE_SYM_CLASS_ENUM_TAG            0x000F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM      0x0010
#define IMAGE_SYM_CLASS_REGISTER_PARAM      0x0011
#define IMAGE_SYM_CLASS_BIT_FIELD           0x0012

#define IMAGE_SYM_CLASS_FAR_EXTERNAL        0x0044  //

#define IMAGE_SYM_CLASS_BLOCK               0x0064
#define IMAGE_SYM_CLASS_FUNCTION            0x0065
#define IMAGE_SYM_CLASS_END_OF_STRUCT       0x0066
#define IMAGE_SYM_CLASS_FILE                0x0067
// new
#define IMAGE_SYM_CLASS_SECTION             0x0068
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL       0x0069

#define IMAGE_SYM_CLASS_CLR_TOKEN           0x006B

// type packing constants

#define N_BTMASK                            0x000F
#define N_TMASK                             0x0030
#define N_TMASK1                            0x00C0
#define N_TMASK2                            0x00F0
#define N_BTSHFT                            4
#define N_TSHIFT                            2
// MACROS

// Basic Type of  x
#define BTYPE(x) ((x) & N_BTMASK)

// Is x a pointer?
#ifndef ISPTR
#define ISPTR(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_POINTER << N_BTSHFT))
#endif

// Is x a function?
#ifndef ISFCN
#define ISFCN(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT))
#endif

// Is x an array?

#ifndef ISARY
#define ISARY(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_ARRAY << N_BTSHFT))
#endif

// Is x a structure, union, or enumeration TAG?
#ifndef ISTAG
#define ISTAG(x) ((x)==IMAGE_SYM_CLASS_STRUCT_TAG || (x)==IMAGE_SYM_CLASS_UNION_TAG || (x)==IMAGE_SYM_CLASS_ENUM_TAG)
#endif

#ifndef INCREF
#define INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(IMAGE_SYM_DTYPE_POINTER<<N_BTSHFT)|((x)&N_BTMASK))
#endif
#ifndef DECREF
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))
#endif

#include <pshpack2.h>

typedef struct IMAGE_AUX_SYMBOL_TOKEN_DEF {
    UCHAR bAuxType;                  // IMAGE_AUX_SYMBOL_TYPE
    UCHAR bReserved;                 // Must be 0
    ULONG SymbolTableIndex;
    UCHAR rgbReserved[12];           // Must be 0
} IMAGE_AUX_SYMBOL_TOKEN_DEF;

typedef IMAGE_AUX_SYMBOL_TOKEN_DEF UNALIGNED *PIMAGE_AUX_SYMBOL_TOKEN_DEF;

#include <poppack.h>

//
// Auxiliary entry format.
//

typedef union _IMAGE_AUX_SYMBOL {
    struct {
        ULONG    TagIndex;                      // struct, union, or enum tag index
        union {
            struct {
                USHORT  Linenumber;             // declaration line number
                USHORT  Size;                   // size of struct, union, or enum
            } LnSz;
           ULONG    TotalSize;
        } Misc;
        union {
            struct {                            // if ISFCN, tag, or .bb
                ULONG    PointerToLinenumber;
                ULONG    PointerToNextFunction;
            } Function;
            struct {                            // if ISARY, up to 4 dimen.
                USHORT   Dimension[4];
            } Array;
        } FcnAry;
        USHORT  TvIndex;                        // tv index
    } Sym;
    struct {
        UCHAR   Name[IMAGE_SIZEOF_SYMBOL];
    } File;
    struct {
        ULONG   Length;                         // section length
        USHORT  NumberOfRelocations;            // number of relocation entries
        USHORT  NumberOfLinenumbers;            // number of line numbers
        ULONG   CheckSum;                       // checksum for communal
        SHORT   Number;                         // section number to associate with
        UCHAR   Selection;                      // communal selection type
	UCHAR   bReserved;
	SHORT   HighNumber;                     // high bits of the section number
    } Section;
    IMAGE_AUX_SYMBOL_TOKEN_DEF TokenDef;
    struct {
        ULONG crc;
        UCHAR rgbReserved[14];
    } CRC;
} IMAGE_AUX_SYMBOL;
typedef IMAGE_AUX_SYMBOL UNALIGNED *PIMAGE_AUX_SYMBOL;

typedef union _IMAGE_AUX_SYMBOL_EX {
    struct {
        ULONG   WeakDefaultSymIndex;                       // the weak extern default symbol index
        ULONG   WeakSearchType;
        UCHAR   rgbReserved[12];
    } Sym;
    struct {
        UCHAR   Name[sizeof(IMAGE_SYMBOL_EX)];
    } File;
    struct {
        ULONG   Length;                         // section length
        USHORT  NumberOfRelocations;            // number of relocation entries
        USHORT  NumberOfLinenumbers;            // number of line numbers
        ULONG   CheckSum;                       // checksum for communal
        SHORT   Number;                         // section number to associate with
        UCHAR   Selection;                      // communal selection type
        UCHAR   bReserved;
        SHORT   HighNumber;                     // high bits of the section number
        UCHAR   rgbReserved[2];
    } Section;
    struct{
        IMAGE_AUX_SYMBOL_TOKEN_DEF TokenDef;
        UCHAR rgbReserved[2];
    } DUMMYSTRUCTNAME;
    struct {
        ULONG crc;
        UCHAR rgbReserved[16];
    } CRC;
} IMAGE_AUX_SYMBOL_EX;
typedef IMAGE_AUX_SYMBOL_EX UNALIGNED *PIMAGE_AUX_SYMBOL_EX;

typedef enum IMAGE_AUX_SYMBOL_TYPE {
    IMAGE_AUX_SYMBOL_TYPE_TOKEN_DEF = 1,
} IMAGE_AUX_SYMBOL_TYPE;

//
// Communal selection types.
//

#define IMAGE_COMDAT_SELECT_NODUPLICATES    1
#define IMAGE_COMDAT_SELECT_ANY             2
#define IMAGE_COMDAT_SELECT_SAME_SIZE       3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH     4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE     5
#define IMAGE_COMDAT_SELECT_LARGEST         6
#define IMAGE_COMDAT_SELECT_NEWEST          7

#define IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY  1
#define IMAGE_WEAK_EXTERN_SEARCH_LIBRARY    2
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS      3
#define IMAGE_WEAK_EXTERN_ANTI_DEPENDENCY   4

//
// Relocation format.
//

typedef struct _IMAGE_RELOCATION {
    union {
        ULONG   VirtualAddress;
        ULONG   RelocCount;             // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
    } DUMMYUNIONNAME;
    ULONG   SymbolTableIndex;
    USHORT  Type;
} IMAGE_RELOCATION;
typedef IMAGE_RELOCATION UNALIGNED *PIMAGE_RELOCATION;

//
// I386 relocation types.
//
#define IMAGE_REL_I386_ABSOLUTE         0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_I386_DIR16            0x0001  // Direct 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_REL16            0x0002  // PC-relative 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32            0x0006  // Direct 32-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32NB          0x0007  // Direct 32-bit reference to the symbols virtual address, base not included
#define IMAGE_REL_I386_SEG12            0x0009  // Direct 16-bit reference to the segment-selector bits of a 32-bit virtual address
#define IMAGE_REL_I386_SECTION          0x000A
#define IMAGE_REL_I386_SECREL           0x000B
#define IMAGE_REL_I386_TOKEN            0x000C  // clr token
#define IMAGE_REL_I386_SECREL7          0x000D  // 7 bit offset from base of section containing target
#define IMAGE_REL_I386_REL32            0x0014  // PC-relative 32-bit reference to the symbols virtual address

//
// MIPS relocation types.
//
#define IMAGE_REL_MIPS_ABSOLUTE         0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_MIPS_REFHALF          0x0001
#define IMAGE_REL_MIPS_REFWORD          0x0002
#define IMAGE_REL_MIPS_JMPADDR          0x0003
#define IMAGE_REL_MIPS_REFHI            0x0004
#define IMAGE_REL_MIPS_REFLO            0x0005
#define IMAGE_REL_MIPS_GPREL            0x0006
#define IMAGE_REL_MIPS_LITERAL          0x0007
#define IMAGE_REL_MIPS_SECTION          0x000A
#define IMAGE_REL_MIPS_SECREL           0x000B
#define IMAGE_REL_MIPS_SECRELLO         0x000C  // Low 16-bit section relative referemce (used for >32k TLS)
#define IMAGE_REL_MIPS_SECRELHI         0x000D  // High 16-bit section relative reference (used for >32k TLS)
#define IMAGE_REL_MIPS_TOKEN            0x000E  // clr token
#define IMAGE_REL_MIPS_JMPADDR16        0x0010
#define IMAGE_REL_MIPS_REFWORDNB        0x0022
#define IMAGE_REL_MIPS_PAIR             0x0025

//
// Alpha Relocation types.
//
#define IMAGE_REL_ALPHA_ABSOLUTE        0x0000
#define IMAGE_REL_ALPHA_REFLONG         0x0001
#define IMAGE_REL_ALPHA_REFQUAD         0x0002
#define IMAGE_REL_ALPHA_GPREL32         0x0003
#define IMAGE_REL_ALPHA_LITERAL         0x0004
#define IMAGE_REL_ALPHA_LITUSE          0x0005
#define IMAGE_REL_ALPHA_GPDISP          0x0006
#define IMAGE_REL_ALPHA_BRADDR          0x0007
#define IMAGE_REL_ALPHA_HINT            0x0008
#define IMAGE_REL_ALPHA_INLINE_REFLONG  0x0009
#define IMAGE_REL_ALPHA_REFHI           0x000A
#define IMAGE_REL_ALPHA_REFLO           0x000B
#define IMAGE_REL_ALPHA_PAIR            0x000C
#define IMAGE_REL_ALPHA_MATCH           0x000D
#define IMAGE_REL_ALPHA_SECTION         0x000E
#define IMAGE_REL_ALPHA_SECREL          0x000F
#define IMAGE_REL_ALPHA_REFLONGNB       0x0010
#define IMAGE_REL_ALPHA_SECRELLO        0x0011  // Low 16-bit section relative reference
#define IMAGE_REL_ALPHA_SECRELHI        0x0012  // High 16-bit section relative reference
#define IMAGE_REL_ALPHA_REFQ3           0x0013  // High 16 bits of 48 bit reference
#define IMAGE_REL_ALPHA_REFQ2           0x0014  // Middle 16 bits of 48 bit reference
#define IMAGE_REL_ALPHA_REFQ1           0x0015  // Low 16 bits of 48 bit reference
#define IMAGE_REL_ALPHA_GPRELLO         0x0016  // Low 16-bit GP relative reference
#define IMAGE_REL_ALPHA_GPRELHI         0x0017  // High 16-bit GP relative reference

//
// IBM PowerPC relocation types.
//
#define IMAGE_REL_PPC_ABSOLUTE          0x0000  // NOP
#define IMAGE_REL_PPC_ADDR64            0x0001  // 64-bit address
#define IMAGE_REL_PPC_ADDR32            0x0002  // 32-bit address
#define IMAGE_REL_PPC_ADDR24            0x0003  // 26-bit address, shifted left 2 (branch absolute)
#define IMAGE_REL_PPC_ADDR16            0x0004  // 16-bit address
#define IMAGE_REL_PPC_ADDR14            0x0005  // 16-bit address, shifted left 2 (load doubleword)
#define IMAGE_REL_PPC_REL24             0x0006  // 26-bit PC-relative offset, shifted left 2 (branch relative)
#define IMAGE_REL_PPC_REL14             0x0007  // 16-bit PC-relative offset, shifted left 2 (br cond relative)
#define IMAGE_REL_PPC_TOCREL16          0x0008  // 16-bit offset from TOC base
#define IMAGE_REL_PPC_TOCREL14          0x0009  // 16-bit offset from TOC base, shifted left 2 (load doubleword)

#define IMAGE_REL_PPC_ADDR32NB          0x000A  // 32-bit addr w/o image base
#define IMAGE_REL_PPC_SECREL            0x000B  // va of containing section (as in an image sectionhdr)
#define IMAGE_REL_PPC_SECTION           0x000C  // sectionheader number
#define IMAGE_REL_PPC_IFGLUE            0x000D  // substitute TOC restore instruction iff symbol is glue code
#define IMAGE_REL_PPC_IMGLUE            0x000E  // symbol is glue code; virtual address is TOC restore instruction
#define IMAGE_REL_PPC_SECREL16          0x000F  // va of containing section (limited to 16 bits)
#define IMAGE_REL_PPC_REFHI             0x0010
#define IMAGE_REL_PPC_REFLO             0x0011
#define IMAGE_REL_PPC_PAIR              0x0012
#define IMAGE_REL_PPC_SECRELLO          0x0013  // Low 16-bit section relative reference (used for >32k TLS)
#define IMAGE_REL_PPC_SECRELHI          0x0014  // High 16-bit section relative reference (used for >32k TLS)
#define IMAGE_REL_PPC_GPREL             0x0015
#define IMAGE_REL_PPC_TOKEN             0x0016  // clr token

#define IMAGE_REL_PPC_TYPEMASK          0x00FF  // mask to isolate above values in IMAGE_RELOCATION.Type

// Flag bits in IMAGE_RELOCATION.TYPE

#define IMAGE_REL_PPC_NEG               0x0100  // subtract reloc value rather than adding it
#define IMAGE_REL_PPC_BRTAKEN           0x0200  // fix branch prediction bit to predict branch taken
#define IMAGE_REL_PPC_BRNTAKEN          0x0400  // fix branch prediction bit to predict branch not taken
#define IMAGE_REL_PPC_TOCDEFN           0x0800  // toc slot defined in file (or, data in toc)

//
// Hitachi SH3 relocation types.
//
#define IMAGE_REL_SH3_ABSOLUTE          0x0000  // No relocation
#define IMAGE_REL_SH3_DIRECT16          0x0001  // 16 bit direct
#define IMAGE_REL_SH3_DIRECT32          0x0002  // 32 bit direct
#define IMAGE_REL_SH3_DIRECT8           0x0003  // 8 bit direct, -128..255
#define IMAGE_REL_SH3_DIRECT8_WORD      0x0004  // 8 bit direct .W (0 ext.)
#define IMAGE_REL_SH3_DIRECT8_LONG      0x0005  // 8 bit direct .L (0 ext.)
#define IMAGE_REL_SH3_DIRECT4           0x0006  // 4 bit direct (0 ext.)
#define IMAGE_REL_SH3_DIRECT4_WORD      0x0007  // 4 bit direct .W (0 ext.)
#define IMAGE_REL_SH3_DIRECT4_LONG      0x0008  // 4 bit direct .L (0 ext.)
#define IMAGE_REL_SH3_PCREL8_WORD       0x0009  // 8 bit PC relative .W
#define IMAGE_REL_SH3_PCREL8_LONG       0x000A  // 8 bit PC relative .L
#define IMAGE_REL_SH3_PCREL12_WORD      0x000B  // 12 LSB PC relative .W
#define IMAGE_REL_SH3_STARTOF_SECTION   0x000C  // Start of EXE section
#define IMAGE_REL_SH3_SIZEOF_SECTION    0x000D  // Size of EXE section
#define IMAGE_REL_SH3_SECTION           0x000E  // Section table index
#define IMAGE_REL_SH3_SECREL            0x000F  // Offset within section
#define IMAGE_REL_SH3_DIRECT32_NB       0x0010  // 32 bit direct not based
#define IMAGE_REL_SH3_GPREL4_LONG       0x0011  // GP-relative addressing
#define IMAGE_REL_SH3_TOKEN             0x0012  // clr token
#define IMAGE_REL_SHM_PCRELPT           0x0013  // Offset from current
                                                //  instruction in longwords
                                                //  if not NOMODE, insert the
                                                //  inverse of the low bit at
                                                //  bit 32 to select PTA/PTB
#define IMAGE_REL_SHM_REFLO             0x0014  // Low bits of 32-bit address
#define IMAGE_REL_SHM_REFHALF           0x0015  // High bits of 32-bit address
#define IMAGE_REL_SHM_RELLO             0x0016  // Low bits of relative reference
#define IMAGE_REL_SHM_RELHALF           0x0017  // High bits of relative reference
#define IMAGE_REL_SHM_PAIR              0x0018  // offset operand for relocation

#define IMAGE_REL_SH_NOMODE             0x8000  // relocation ignores section mode

#define IMAGE_REL_ARM_ABSOLUTE          0x0000  // No relocation required
#define IMAGE_REL_ARM_ADDR32            0x0001  // 32 bit address
#define IMAGE_REL_ARM_ADDR32NB          0x0002  // 32 bit address w/o image base
#define IMAGE_REL_ARM_BRANCH24          0x0003  // 24 bit offset << 2 & sign ext.
#define IMAGE_REL_ARM_BRANCH11          0x0004  // Thumb: 2 11 bit offsets
#define IMAGE_REL_ARM_TOKEN             0x0005  // clr token
#define IMAGE_REL_ARM_GPREL12           0x0006  // GP-relative addressing (ARM)
#define IMAGE_REL_ARM_GPREL7            0x0007  // GP-relative addressing (Thumb)
#define IMAGE_REL_ARM_BLX24             0x0008
#define IMAGE_REL_ARM_BLX11             0x0009
#define IMAGE_REL_ARM_SECTION           0x000E  // Section table index
#define IMAGE_REL_ARM_SECREL            0x000F  // Offset within section
#define IMAGE_REL_ARM_MOV32A            0x0010  // ARM: MOVW/MOVT
#define IMAGE_REL_ARM_MOV32             0x0010  // ARM: MOVW/MOVT (deprecated)
#define IMAGE_REL_ARM_MOV32T            0x0011  // Thumb: MOVW/MOVT
#define IMAGE_REL_THUMB_MOV32           0x0011  // Thumb: MOVW/MOVT (deprecated)
#define IMAGE_REL_ARM_BRANCH20T         0x0012  // Thumb: 32-bit conditional B
#define IMAGE_REL_THUMB_BRANCH20        0x0012  // Thumb: 32-bit conditional B (deprecated)
#define IMAGE_REL_ARM_BRANCH24T         0x0014  // Thumb: 32-bit B or BL
#define IMAGE_REL_THUMB_BRANCH24        0x0014  // Thumb: 32-bit B or BL (deprecated)
#define IMAGE_REL_ARM_BLX23T            0x0015  // Thumb: BLX immediate
#define IMAGE_REL_THUMB_BLX23           0x0015  // Thumb: BLX immediate (deprecated)

#define IMAGE_REL_AM_ABSOLUTE           0x0000
#define IMAGE_REL_AM_ADDR32             0x0001
#define IMAGE_REL_AM_ADDR32NB           0x0002
#define IMAGE_REL_AM_CALL32             0x0003
#define IMAGE_REL_AM_FUNCINFO           0x0004
#define IMAGE_REL_AM_REL32_1            0x0005
#define IMAGE_REL_AM_REL32_2            0x0006
#define IMAGE_REL_AM_SECREL             0x0007
#define IMAGE_REL_AM_SECTION            0x0008
#define IMAGE_REL_AM_TOKEN              0x0009

//
// ARM64 relocations types.
//

#define IMAGE_REL_ARM64_ABSOLUTE        0x0000  // No relocation required
#define IMAGE_REL_ARM64_ADDR32          0x0001  // 32 bit address. Review! do we need it?
#define IMAGE_REL_ARM64_ADDR32NB        0x0002  // 32 bit address w/o image base (RVA: for Data/PData/XData)
#define IMAGE_REL_ARM64_BRANCH26        0x0003  // 26 bit offset << 2 & sign ext. for B & BL
#define IMAGE_REL_ARM64_PAGEBASE_REL21  0x0004  // ADRP
#define IMAGE_REL_ARM64_REL21           0x0005  // ADR
#define IMAGE_REL_ARM64_PAGEOFFSET_12A  0x0006  // ADD/ADDS (immediate) with zero shift, for page offset
#define IMAGE_REL_ARM64_PAGEOFFSET_12L  0x0007  // LDR (indexed, unsigned immediate), for page offset
#define IMAGE_REL_ARM64_SECREL          0x0008  // Offset within section
#define IMAGE_REL_ARM64_SECREL_LOW12A   0x0009  // ADD/ADDS (immediate) with zero shift, for bit 0:11 of section offset
#define IMAGE_REL_ARM64_SECREL_HIGH12A  0x000A  // ADD/ADDS (immediate) with zero shift, for bit 12:23 of section offset
#define IMAGE_REL_ARM64_SECREL_LOW12L   0x000B  // LDR (indexed, unsigned immediate), for bit 0:11 of section offset
#define IMAGE_REL_ARM64_TOKEN           0x000C
#define IMAGE_REL_ARM64_SECTION         0x000D  // Section table index
#define IMAGE_REL_ARM64_ADDR64          0x000E  // 64 bit address
#define IMAGE_REL_ARM64_BRANCH19        0x000F  // 19 bit offset << 2 & sign ext. for conditional B

//
// x64 relocations
//
#define IMAGE_REL_AMD64_ABSOLUTE        0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_AMD64_ADDR64          0x0001  // 64-bit address (VA).
#define IMAGE_REL_AMD64_ADDR32          0x0002  // 32-bit address (VA).
#define IMAGE_REL_AMD64_ADDR32NB        0x0003  // 32-bit address w/o image base (RVA).
#define IMAGE_REL_AMD64_REL32           0x0004  // 32-bit relative address from byte following reloc
#define IMAGE_REL_AMD64_REL32_1         0x0005  // 32-bit relative address from byte distance 1 from reloc
#define IMAGE_REL_AMD64_REL32_2         0x0006  // 32-bit relative address from byte distance 2 from reloc
#define IMAGE_REL_AMD64_REL32_3         0x0007  // 32-bit relative address from byte distance 3 from reloc
#define IMAGE_REL_AMD64_REL32_4         0x0008  // 32-bit relative address from byte distance 4 from reloc
#define IMAGE_REL_AMD64_REL32_5         0x0009  // 32-bit relative address from byte distance 5 from reloc
#define IMAGE_REL_AMD64_SECTION         0x000A  // Section index
#define IMAGE_REL_AMD64_SECREL          0x000B  // 32 bit offset from base of section containing target
#define IMAGE_REL_AMD64_SECREL7         0x000C  // 7 bit unsigned offset from base of section containing target
#define IMAGE_REL_AMD64_TOKEN           0x000D  // 32 bit metadata token
#define IMAGE_REL_AMD64_SREL32          0x000E  // 32 bit signed span-dependent value emitted into object
#define IMAGE_REL_AMD64_PAIR            0x000F
#define IMAGE_REL_AMD64_SSPAN32         0x0010  // 32 bit signed span-dependent value applied at link time
#define IMAGE_REL_AMD64_EHANDLER        0x0011
#define IMAGE_REL_AMD64_IMPORT_BR       0x0012  // Indirect branch to an import
#define IMAGE_REL_AMD64_IMPORT_CALL     0x0013  // Indirect call to an import
#define IMAGE_REL_AMD64_CFG_BR          0x0014  // Indirect branch to a CFG check
#define IMAGE_REL_AMD64_CFG_BR_REX      0x0015  // Indirect branch to a CFG check, with REX.W prefix
#define IMAGE_REL_AMD64_CFG_CALL        0x0016  // Indirect call to a CFG check
#define IMAGE_REL_AMD64_INDIR_BR        0x0017  // Indirect branch to a target in RAX (no CFG)
#define IMAGE_REL_AMD64_INDIR_BR_REX    0x0018  // Indirect branch to a target in RAX, with REX.W prefix (no CFG)
#define IMAGE_REL_AMD64_INDIR_CALL      0x0019  // Indirect call to a target in RAX (no CFG)
#define IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_FIRST  0x0020 // Indirect branch for a switch table using Reg 0 (RAX)
#define IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_LAST   0x002F // Indirect branch for a switch table using Reg 15 (R15)

//
// IA64 relocation types.
//
#define IMAGE_REL_IA64_ABSOLUTE         0x0000
#define IMAGE_REL_IA64_IMM14            0x0001
#define IMAGE_REL_IA64_IMM22            0x0002
#define IMAGE_REL_IA64_IMM64            0x0003
#define IMAGE_REL_IA64_DIR32            0x0004
#define IMAGE_REL_IA64_DIR64            0x0005
#define IMAGE_REL_IA64_PCREL21B         0x0006
#define IMAGE_REL_IA64_PCREL21M         0x0007
#define IMAGE_REL_IA64_PCREL21F         0x0008
#define IMAGE_REL_IA64_GPREL22          0x0009
#define IMAGE_REL_IA64_LTOFF22          0x000A
#define IMAGE_REL_IA64_SECTION          0x000B
#define IMAGE_REL_IA64_SECREL22         0x000C
#define IMAGE_REL_IA64_SECREL64I        0x000D
#define IMAGE_REL_IA64_SECREL32         0x000E
//
#define IMAGE_REL_IA64_DIR32NB          0x0010
#define IMAGE_REL_IA64_SREL14           0x0011
#define IMAGE_REL_IA64_SREL22           0x0012
#define IMAGE_REL_IA64_SREL32           0x0013
#define IMAGE_REL_IA64_UREL32           0x0014
#define IMAGE_REL_IA64_PCREL60X         0x0015  // This is always a BRL and never converted
#define IMAGE_REL_IA64_PCREL60B         0x0016  // If possible, convert to MBB bundle with NOP.B in slot 1
#define IMAGE_REL_IA64_PCREL60F         0x0017  // If possible, convert to MFB bundle with NOP.F in slot 1
#define IMAGE_REL_IA64_PCREL60I         0x0018  // If possible, convert to MIB bundle with NOP.I in slot 1
#define IMAGE_REL_IA64_PCREL60M         0x0019  // If possible, convert to MMB bundle with NOP.M in slot 1
#define IMAGE_REL_IA64_IMMGPREL64       0x001A
#define IMAGE_REL_IA64_TOKEN            0x001B  // clr token
#define IMAGE_REL_IA64_GPREL32          0x001C
#define IMAGE_REL_IA64_ADDEND           0x001F

//
// CEF relocation types.
//
#define IMAGE_REL_CEF_ABSOLUTE          0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_CEF_ADDR32            0x0001  // 32-bit address (VA).
#define IMAGE_REL_CEF_ADDR64            0x0002  // 64-bit address (VA).
#define IMAGE_REL_CEF_ADDR32NB          0x0003  // 32-bit address w/o image base (RVA).
#define IMAGE_REL_CEF_SECTION           0x0004  // Section index
#define IMAGE_REL_CEF_SECREL            0x0005  // 32 bit offset from base of section containing target
#define IMAGE_REL_CEF_TOKEN             0x0006  // 32 bit metadata token

//
// clr relocation types.
//
#define IMAGE_REL_CEE_ABSOLUTE          0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_CEE_ADDR32            0x0001  // 32-bit address (VA).
#define IMAGE_REL_CEE_ADDR64            0x0002  // 64-bit address (VA).
#define IMAGE_REL_CEE_ADDR32NB          0x0003  // 32-bit address w/o image base (RVA).
#define IMAGE_REL_CEE_SECTION           0x0004  // Section index
#define IMAGE_REL_CEE_SECREL            0x0005  // 32 bit offset from base of section containing target
#define IMAGE_REL_CEE_TOKEN             0x0006  // 32 bit metadata token

#define IMAGE_REL_M32R_ABSOLUTE         0x0000  // No relocation required
#define IMAGE_REL_M32R_ADDR32           0x0001  // 32 bit address
#define IMAGE_REL_M32R_ADDR32NB         0x0002  // 32 bit address w/o image base
#define IMAGE_REL_M32R_ADDR24           0x0003  // 24 bit address
#define IMAGE_REL_M32R_GPREL16          0x0004  // GP relative addressing
#define IMAGE_REL_M32R_PCREL24          0x0005  // 24 bit offset << 2 & sign ext.
#define IMAGE_REL_M32R_PCREL16          0x0006  // 16 bit offset << 2 & sign ext.
#define IMAGE_REL_M32R_PCREL8           0x0007  // 8 bit offset << 2 & sign ext.
#define IMAGE_REL_M32R_REFHALF          0x0008  // 16 MSBs
#define IMAGE_REL_M32R_REFHI            0x0009  // 16 MSBs; adj for LSB sign ext.
#define IMAGE_REL_M32R_REFLO            0x000A  // 16 LSBs
#define IMAGE_REL_M32R_PAIR             0x000B  // Link HI and LO
#define IMAGE_REL_M32R_SECTION          0x000C  // Section table index
#define IMAGE_REL_M32R_SECREL32         0x000D  // 32 bit section relative reference
#define IMAGE_REL_M32R_TOKEN            0x000E  // clr token

#define IMAGE_REL_EBC_ABSOLUTE          0x0000  // No relocation required
#define IMAGE_REL_EBC_ADDR32NB          0x0001  // 32 bit address w/o image base
#define IMAGE_REL_EBC_REL32             0x0002  // 32-bit relative address from byte following reloc
#define IMAGE_REL_EBC_SECTION           0x0003  // Section table index
#define IMAGE_REL_EBC_SECREL            0x0004  // Offset within section

#define EXT_IMM64(Value, Address, Size, InstPos, ValPos)  /* Intel-IA64-Filler */           \
    Value |= (((ULONGLONG)((*(Address) >> InstPos) & (((ULONGLONG)1 << Size) - 1))) << ValPos)  // Intel-IA64-Filler

#define INS_IMM64(Value, Address, Size, InstPos, ValPos)  /* Intel-IA64-Filler */\
    *(PULONG)Address = (*(PULONG)Address & ~(((1 << Size) - 1) << InstPos)) | /* Intel-IA64-Filler */\
          ((ULONG)((((ULONGLONG)Value >> ValPos) & (((ULONGLONG)1 << Size) - 1))) << InstPos)  // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM7B_INST_WORD_X         3  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM7B_SIZE_X              7  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X     4  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM7B_VAL_POS_X           0  // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM9D_INST_WORD_X         3  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM9D_SIZE_X              9  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X     18 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM9D_VAL_POS_X           7  // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM5C_INST_WORD_X         3  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM5C_SIZE_X              5  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X     13 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM5C_VAL_POS_X           16 // Intel-IA64-Filler

#define EMARCH_ENC_I17_IC_INST_WORD_X            3  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IC_SIZE_X                 1  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IC_INST_WORD_POS_X        12 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IC_VAL_POS_X              21 // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM41a_INST_WORD_X        1  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41a_SIZE_X             10 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X    14 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41a_VAL_POS_X          22 // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM41b_INST_WORD_X        1  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41b_SIZE_X             8  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X    24 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41b_VAL_POS_X          32 // Intel-IA64-Filler

#define EMARCH_ENC_I17_IMM41c_INST_WORD_X        2  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41c_SIZE_X             23 // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X    0  // Intel-IA64-Filler
#define EMARCH_ENC_I17_IMM41c_VAL_POS_X          40 // Intel-IA64-Filler

#define EMARCH_ENC_I17_SIGN_INST_WORD_X          3  // Intel-IA64-Filler
#define EMARCH_ENC_I17_SIGN_SIZE_X               1  // Intel-IA64-Filler
#define EMARCH_ENC_I17_SIGN_INST_WORD_POS_X      27 // Intel-IA64-Filler
#define EMARCH_ENC_I17_SIGN_VAL_POS_X            63 // Intel-IA64-Filler

#define X3_OPCODE_INST_WORD_X                    3  // Intel-IA64-Filler
#define X3_OPCODE_SIZE_X                         4  // Intel-IA64-Filler
#define X3_OPCODE_INST_WORD_POS_X                28 // Intel-IA64-Filler
#define X3_OPCODE_SIGN_VAL_POS_X                 0  // Intel-IA64-Filler

#define X3_I_INST_WORD_X                         3  // Intel-IA64-Filler
#define X3_I_SIZE_X                              1  // Intel-IA64-Filler
#define X3_I_INST_WORD_POS_X                     27 // Intel-IA64-Filler
#define X3_I_SIGN_VAL_POS_X                      59 // Intel-IA64-Filler

#define X3_D_WH_INST_WORD_X                      3  // Intel-IA64-Filler
#define X3_D_WH_SIZE_X                           3  // Intel-IA64-Filler
#define X3_D_WH_INST_WORD_POS_X                  24 // Intel-IA64-Filler
#define X3_D_WH_SIGN_VAL_POS_X                   0  // Intel-IA64-Filler

#define X3_IMM20_INST_WORD_X                     3  // Intel-IA64-Filler
#define X3_IMM20_SIZE_X                          20 // Intel-IA64-Filler
#define X3_IMM20_INST_WORD_POS_X                 4  // Intel-IA64-Filler
#define X3_IMM20_SIGN_VAL_POS_X                  0  // Intel-IA64-Filler

#define X3_IMM39_1_INST_WORD_X                   2  // Intel-IA64-Filler
#define X3_IMM39_1_SIZE_X                        23 // Intel-IA64-Filler
#define X3_IMM39_1_INST_WORD_POS_X               0  // Intel-IA64-Filler
#define X3_IMM39_1_SIGN_VAL_POS_X                36 // Intel-IA64-Filler

#define X3_IMM39_2_INST_WORD_X                   1  // Intel-IA64-Filler
#define X3_IMM39_2_SIZE_X                        16 // Intel-IA64-Filler
#define X3_IMM39_2_INST_WORD_POS_X               16 // Intel-IA64-Filler
#define X3_IMM39_2_SIGN_VAL_POS_X                20 // Intel-IA64-Filler

#define X3_P_INST_WORD_X                         3  // Intel-IA64-Filler
#define X3_P_SIZE_X                              4  // Intel-IA64-Filler
#define X3_P_INST_WORD_POS_X                     0  // Intel-IA64-Filler
#define X3_P_SIGN_VAL_POS_X                      0  // Intel-IA64-Filler

#define X3_TMPLT_INST_WORD_X                     0  // Intel-IA64-Filler
#define X3_TMPLT_SIZE_X                          4  // Intel-IA64-Filler
#define X3_TMPLT_INST_WORD_POS_X                 0  // Intel-IA64-Filler
#define X3_TMPLT_SIGN_VAL_POS_X                  0  // Intel-IA64-Filler

#define X3_BTYPE_QP_INST_WORD_X                  2  // Intel-IA64-Filler
#define X3_BTYPE_QP_SIZE_X                       9  // Intel-IA64-Filler
#define X3_BTYPE_QP_INST_WORD_POS_X              23 // Intel-IA64-Filler
#define X3_BTYPE_QP_INST_VAL_POS_X               0  // Intel-IA64-Filler

#define X3_EMPTY_INST_WORD_X                     1  // Intel-IA64-Filler
#define X3_EMPTY_SIZE_X                          2  // Intel-IA64-Filler
#define X3_EMPTY_INST_WORD_POS_X                 14 // Intel-IA64-Filler
#define X3_EMPTY_INST_VAL_POS_X                  0  // Intel-IA64-Filler

//
// Line number format.
//

typedef struct _IMAGE_LINENUMBER {
    union {
        ULONG   SymbolTableIndex;               // Symbol table index of function name if Linenumber is 0.
        ULONG   VirtualAddress;                 // Virtual address of line number.
    } Type;
    USHORT  Linenumber;                         // Line number.
} IMAGE_LINENUMBER;
typedef IMAGE_LINENUMBER UNALIGNED *PIMAGE_LINENUMBER;

#ifndef _MAC
#include "poppack.h"                        // Back to 4 byte packing
#endif

//
// Based relocation format.
//

typedef struct _IMAGE_BASE_RELOCATION {
    ULONG VirtualAddress;
    ULONG SizeOfBlock;
} IMAGE_BASE_RELOCATION;

typedef IMAGE_BASE_RELOCATION UNALIGNED * PIMAGE_BASE_RELOCATION;

//
// Based relocation types.
//

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_5    5
#define IMAGE_REL_BASED_RESERVED              6
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_7    7
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_8    8
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_9    9
#define IMAGE_REL_BASED_DIR64                 10

//
// Platform-specific based relocation types.
//

#define IMAGE_REL_BASED_IA64_IMM64            9

#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_MIPS_JMPADDR16        9

#define IMAGE_REL_BASED_ARM_MOV32             5
#define IMAGE_REL_BASED_THUMB_MOV32           7

//
// Archive format.
//

#define IMAGE_ARCHIVE_START_SIZE             8
#define IMAGE_ARCHIVE_START                  "!<arch>\n"
#define IMAGE_ARCHIVE_END                    "`\n"
#define IMAGE_ARCHIVE_PAD                    "\n"
#define IMAGE_ARCHIVE_LINKER_MEMBER          "/               "
#define IMAGE_ARCHIVE_LONGNAMES_MEMBER       "//              "
#define IMAGE_ARCHIVE_HYBRIDMAP_MEMBER       "/<HYBRIDMAP>/   "

typedef struct _IMAGE_ARCHIVE_MEMBER_HEADER {
    UCHAR    Name[16];                          // File member name - `/' terminated.
    UCHAR    Date[12];                          // File member date - decimal.
    UCHAR    UserID[6];                         // File member user id - decimal.
    UCHAR    GroupID[6];                        // File member group id - decimal.
    UCHAR    Mode[8];                           // File member mode - octal.
    UCHAR    Size[10];                          // File member size - decimal.
    UCHAR    EndHeader[2];                      // String to end header.
} IMAGE_ARCHIVE_MEMBER_HEADER, *PIMAGE_ARCHIVE_MEMBER_HEADER;

#define IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR      60

//
// DLL support.
//

//
// Export Format
//

typedef struct _IMAGE_EXPORT_DIRECTORY {
    ULONG Characteristics;
    ULONG TimeDateStamp;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Name;
    ULONG Base;
    ULONG NumberOfFunctions;
    ULONG NumberOfNames;
    ULONG AddressOfFunctions;
    ULONG AddressOfNames;
    ULONG AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

//
// Import Format
//

typedef struct _IMAGE_IMPORT_BY_NAME {
    USHORT Hint;
    CHAR Name [1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

#include "pshpack8.h"                       // Use align 8 for the 64-bit IAT.

typedef struct _IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64;

typedef IMAGE_THUNK_DATA64 * PIMAGE_THUNK_DATA64;

#include "poppack.h"                        // Back to 4 byte packing

typedef struct _IMAGE_THUNK_DATA32 {
    union {
        ULONG ForwarderString;
        ULONG Function;
        ULONG Ordinal;
        ULONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA32;

typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32) != 0)

//
// Thread Local Storage
//

typedef VOID
(NTAPI *PIMAGE_TLS_CALLBACK) (
    PVOID DllHandle,
    ULONG Reason,
    PVOID Reserved
    );

typedef struct _IMAGE_TLS_DIRECTORY64 {
    ULONGLONG StartAddressOfRawData;
    ULONGLONG EndAddressOfRawData;
    ULONGLONG AddressOfIndex;         // PULONG
    ULONGLONG AddressOfCallBacks;     // PIMAGE_TLS_CALLBACK *;
    ULONG SizeOfZeroFill;
    union {
        ULONG Characteristics;
        struct {
            ULONG Reserved0 : 20;
            ULONG Alignment : 4;
            ULONG Reserved1 : 8;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

} IMAGE_TLS_DIRECTORY64;

typedef IMAGE_TLS_DIRECTORY64 * PIMAGE_TLS_DIRECTORY64;

typedef struct _IMAGE_TLS_DIRECTORY32 {
    ULONG   StartAddressOfRawData;
    ULONG   EndAddressOfRawData;
    ULONG   AddressOfIndex;             // PULONG
    ULONG   AddressOfCallBacks;         // PIMAGE_TLS_CALLBACK *
    ULONG   SizeOfZeroFill;
    union {
        ULONG Characteristics;
        struct {
            ULONG Reserved0 : 20;
            ULONG Alignment : 4;
            ULONG Reserved1 : 8;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

} IMAGE_TLS_DIRECTORY32;
typedef IMAGE_TLS_DIRECTORY32 * PIMAGE_TLS_DIRECTORY32;

#ifdef _WIN64
#define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG64
#define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL64(Ordinal)
typedef IMAGE_THUNK_DATA64              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA64             PIMAGE_THUNK_DATA;
#define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL64(Ordinal)
typedef IMAGE_TLS_DIRECTORY64           IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY64          PIMAGE_TLS_DIRECTORY;
#else
#define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG32
#define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL32(Ordinal)
typedef IMAGE_THUNK_DATA32              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32             PIMAGE_THUNK_DATA;
#define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL32(Ordinal)
typedef IMAGE_TLS_DIRECTORY32           IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY32          PIMAGE_TLS_DIRECTORY;
#endif

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        ULONG Characteristics;
        ULONG OriginalFirstThunk;
    } DUMMYUNIONNAME;
    ULONG TimeDateStamp;
    ULONG ForwarderChain;
    ULONG Name;
    ULONG FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;

typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;

//
// New format import descriptors pointed to by DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ]
//

typedef struct _IMAGE_BOUND_IMPORT_DESCRIPTOR {
    ULONG   TimeDateStamp;
    USHORT  OffsetModuleName;
    USHORT  NumberOfModuleForwarderRefs;
// Array of zero or more IMAGE_BOUND_FORWARDER_REF follows
} IMAGE_BOUND_IMPORT_DESCRIPTOR,  *PIMAGE_BOUND_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_BOUND_FORWARDER_REF {
    ULONG   TimeDateStamp;
    USHORT  OffsetModuleName;
    USHORT  Reserved;
} IMAGE_BOUND_FORWARDER_REF, *PIMAGE_BOUND_FORWARDER_REF;

typedef struct _IMAGE_DELAYLOAD_DESCRIPTOR {
    union {
        ULONG AllAttributes;
        struct {
            ULONG RvaBased : 1;             // Delay load version 2
            ULONG ReservedAttributes : 31;
        } DUMMYSTRUCTNAME;
    } Attributes;

    ULONG DllNameRVA;                       // RVA to the name of the target library (NULL-terminate ASCII string)
    ULONG ModuleHandleRVA;                  // RVA to the HMODULE caching location (PHMODULE)
    ULONG ImportAddressTableRVA;            // RVA to the start of the IAT (PIMAGE_THUNK_DATA)
    ULONG ImportNameTableRVA;               // RVA to the start of the name table (PIMAGE_THUNK_DATA::AddressOfData)
    ULONG BoundImportAddressTableRVA;       // RVA to an optional bound IAT
    ULONG UnloadInformationTableRVA;        // RVA to an optional unload info table
    ULONG TimeDateStamp;                    // 0 if not bound,
                                            // Otherwise, date/time of the target DLL

} IMAGE_DELAYLOAD_DESCRIPTOR, *PIMAGE_DELAYLOAD_DESCRIPTOR;

typedef const IMAGE_DELAYLOAD_DESCRIPTOR *PCIMAGE_DELAYLOAD_DESCRIPTOR;

//
// Resource Format.
//

//
// Resource directory consists of two counts, following by a variable length
// array of directory entries.  The first count is the number of entries at
// beginning of the array that have actual names associated with each entry.
// The entries are in ascending order, case insensitive strings.  The second
// count is the number of entries that immediately follow the named entries.
// This second count identifies the number of entries that have 16-bit integer
// Ids as their name.  These entries are also sorted in ascending order.
//
// This structure allows fast lookup by either name or number, but for any
// given resource entry only one form of lookup is supported, not both.
// This is consistant with the syntax of the .RC file and the .RES file.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY {
    ULONG   Characteristics;
    ULONG   TimeDateStamp;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  NumberOfNamedEntries;
    USHORT  NumberOfIdEntries;
//  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000
//
// Each directory contains the 32-bit Name of the entry and an offset,
// relative to the beginning of the resource directory of the data associated
// with this directory entry.  If the name of the entry is an actual text
// string instead of an integer Id, then the high order bit of the name field
// is set to one and the low order 31-bits are an offset, relative to the
// beginning of the resource directory of the string, which is of type
// IMAGE_RESOURCE_DIRECTORY_STRING.  Otherwise the high bit is clear and the
// low-order 16-bits are the integer Id that identify this resource directory
// entry. If the directory entry is yet another resource directory (i.e. a
// subdirectory), then the high order bit of the offset field will be
// set to indicate this.  Otherwise the high bit is clear and the offset
// field points to a resource data entry.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
    union {
        struct {
            ULONG NameOffset:31;
            ULONG NameIsString:1;
        } DUMMYSTRUCTNAME;
        ULONG Name;
        USHORT Id;
    } DUMMYUNIONNAME;
    union {
        ULONG OffsetToData;
        struct {
            ULONG OffsetToDirectory:31;
            ULONG DataIsDirectory:1;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

//
// For resource directory entries that have actual string names, the Name
// field of the directory entry points to an object of the following type.
// All of these string objects are stored together after the last resource
// directory entry and before the first resource data object.  This minimizes
// the impact of these variable length objects on the alignment of the fixed
// size directory entry objects.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
    USHORT  Length;
    CHAR    NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING, *PIMAGE_RESOURCE_DIRECTORY_STRING;

typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
    USHORT  Length;
    WCHAR   NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U, *PIMAGE_RESOURCE_DIR_STRING_U;

//
// Each resource data entry describes a leaf node in the resource directory
// tree.  It contains an offset, relative to the beginning of the resource
// directory of the data for the resource, a size field that gives the number
// of bytes of data at that offset, a CodePage that should be used when
// decoding code point values within the resource data.  Typically for new
// applications the code page would be the unicode code page.
//

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
    ULONG OffsetToData;
    ULONG Size;
    ULONG CodePage;
    ULONG Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;

// begin_ntoshvp

//
// Code Integrity in loadconfig (CI)
//

typedef struct _IMAGE_LOAD_CONFIG_CODE_INTEGRITY {
    USHORT  Flags;          // Flags to indicate if CI information is available, etc.
    USHORT  Catalog;        // 0xFFFF means not available
    ULONG   CatalogOffset;
    ULONG   Reserved;       // Additional bitmask to be defined later
} IMAGE_LOAD_CONFIG_CODE_INTEGRITY, *PIMAGE_LOAD_CONFIG_CODE_INTEGRITY;

//
// Dynamic value relocation table in loadconfig
//

typedef struct _IMAGE_DYNAMIC_RELOCATION_TABLE {
    ULONG Version;
    ULONG Size;
//  IMAGE_DYNAMIC_RELOCATION DynamicRelocations[0];
} IMAGE_DYNAMIC_RELOCATION_TABLE, *PIMAGE_DYNAMIC_RELOCATION_TABLE;

//
// Dynamic value relocation entries following IMAGE_DYNAMIC_RELOCATION_TABLE
//

#include "pshpack1.h"

typedef struct _IMAGE_DYNAMIC_RELOCATION32 {
    ULONG      Symbol;
    ULONG      BaseRelocSize;
//  IMAGE_BASE_RELOCATION BaseRelocations[0];
} IMAGE_DYNAMIC_RELOCATION32, *PIMAGE_DYNAMIC_RELOCATION32;

typedef struct _IMAGE_DYNAMIC_RELOCATION64 {
    ULONGLONG  Symbol;
    ULONG      BaseRelocSize;
//  IMAGE_BASE_RELOCATION BaseRelocations[0];
} IMAGE_DYNAMIC_RELOCATION64, *PIMAGE_DYNAMIC_RELOCATION64;

typedef struct _IMAGE_DYNAMIC_RELOCATION32_V2 {
    ULONG      HeaderSize;
    ULONG      FixupInfoSize;
    ULONG      Symbol;
    ULONG      SymbolGroup;
    ULONG      Flags;
    // ...     variable length header fields
    // UCHAR   FixupInfo[FixupInfoSize]
} IMAGE_DYNAMIC_RELOCATION32_V2, *PIMAGE_DYNAMIC_RELOCATION32_V2;

typedef struct _IMAGE_DYNAMIC_RELOCATION64_V2 {
    ULONG      HeaderSize;
    ULONG      FixupInfoSize;
    ULONGLONG  Symbol;
    ULONG      SymbolGroup;
    ULONG      Flags;
    // ...     variable length header fields
    // UCHAR   FixupInfo[FixupInfoSize]
} IMAGE_DYNAMIC_RELOCATION64_V2, *PIMAGE_DYNAMIC_RELOCATION64_V2;

#include "poppack.h"                    // Back to 4 byte packing

#ifdef _WIN64
typedef IMAGE_DYNAMIC_RELOCATION64          IMAGE_DYNAMIC_RELOCATION;
typedef PIMAGE_DYNAMIC_RELOCATION64         PIMAGE_DYNAMIC_RELOCATION;
typedef IMAGE_DYNAMIC_RELOCATION64_V2       IMAGE_DYNAMIC_RELOCATION_V2;
typedef PIMAGE_DYNAMIC_RELOCATION64_V2      PIMAGE_DYNAMIC_RELOCATION_V2;
#else
typedef IMAGE_DYNAMIC_RELOCATION32          IMAGE_DYNAMIC_RELOCATION;
typedef PIMAGE_DYNAMIC_RELOCATION32         PIMAGE_DYNAMIC_RELOCATION;
typedef IMAGE_DYNAMIC_RELOCATION32_V2       IMAGE_DYNAMIC_RELOCATION_V2;
typedef PIMAGE_DYNAMIC_RELOCATION32_V2      PIMAGE_DYNAMIC_RELOCATION_V2;
#endif

//
// Defined symbolic dynamic relocation entries.
//

#define IMAGE_DYNAMIC_RELOCATION_GUARD_RF_PROLOGUE   0x00000001
#define IMAGE_DYNAMIC_RELOCATION_GUARD_RF_EPILOGUE   0x00000002
#define IMAGE_DYNAMIC_RELOCATION_GUARD_IMPORT_CONTROL_TRANSFER  0x00000003
#define IMAGE_DYNAMIC_RELOCATION_GUARD_INDIR_CONTROL_TRANSFER   0x00000004
#define IMAGE_DYNAMIC_RELOCATION_GUARD_SWITCHTABLE_BRANCH       0x00000005
// end_winnt end_ntoshvp
#define IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA         0x7FFE0000
#define IMAGE_DYNAMIC_RELOCATION_KI_USER_SHARED_DATA64          0xFFFFF78000000000UI64
// begin_winnt begin_ntoshvp

#include "pshpack1.h"

typedef struct _IMAGE_PROLOGUE_DYNAMIC_RELOCATION_HEADER {
    UCHAR      PrologueByteCount;
    // UCHAR   PrologueBytes[PrologueByteCount];
} IMAGE_PROLOGUE_DYNAMIC_RELOCATION_HEADER;
typedef IMAGE_PROLOGUE_DYNAMIC_RELOCATION_HEADER UNALIGNED * PIMAGE_PROLOGUE_DYNAMIC_RELOCATION_HEADER;

typedef struct _IMAGE_EPILOGUE_DYNAMIC_RELOCATION_HEADER {
    ULONG      EpilogueCount;
    UCHAR      EpilogueByteCount;
    UCHAR      BranchDescriptorElementSize;
    USHORT     BranchDescriptorCount;
    // UCHAR   BranchDescriptors[...];
    // UCHAR   BranchDescriptorBitMap[...];
} IMAGE_EPILOGUE_DYNAMIC_RELOCATION_HEADER;
typedef IMAGE_EPILOGUE_DYNAMIC_RELOCATION_HEADER UNALIGNED * PIMAGE_EPILOGUE_DYNAMIC_RELOCATION_HEADER;

typedef struct _IMAGE_IMPORT_CONTROL_TRANSFER_DYNAMIC_RELOCATION {
    ULONG       PageRelativeOffset : 12;
    ULONG       IndirectCall       : 1;
    ULONG       IATIndex           : 19;
} IMAGE_IMPORT_CONTROL_TRANSFER_DYNAMIC_RELOCATION;
typedef IMAGE_IMPORT_CONTROL_TRANSFER_DYNAMIC_RELOCATION UNALIGNED * PIMAGE_IMPORT_CONTROL_TRANSFER_DYNAMIC_RELOCATION;

typedef struct _IMAGE_INDIR_CONTROL_TRANSFER_DYNAMIC_RELOCATION {
    USHORT      PageRelativeOffset : 12;
    USHORT      IndirectCall       : 1;
    USHORT      RexWPrefix         : 1;
    USHORT      CfgCheck           : 1;
    USHORT      Reserved           : 1;
} IMAGE_INDIR_CONTROL_TRANSFER_DYNAMIC_RELOCATION;
typedef IMAGE_INDIR_CONTROL_TRANSFER_DYNAMIC_RELOCATION UNALIGNED * PIMAGE_INDIR_CONTROL_TRANSFER_DYNAMIC_RELOCATION;

typedef struct _IMAGE_SWITCHTABLE_BRANCH_DYNAMIC_RELOCATION {
    USHORT      PageRelativeOffset : 12;
    USHORT      RegisterNumber     : 4;
} IMAGE_SWITCHTABLE_BRANCH_DYNAMIC_RELOCATION;
typedef IMAGE_SWITCHTABLE_BRANCH_DYNAMIC_RELOCATION UNALIGNED * PIMAGE_SWITCHTABLE_BRANCH_DYNAMIC_RELOCATION;

#include "poppack.h"                    // Back to 4 byte packing

//
// Load Configuration Directory Entry
//

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY32 {
    ULONG   Size;
    ULONG   TimeDateStamp;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    ULONG   GlobalFlagsClear;
    ULONG   GlobalFlagsSet;
    ULONG   CriticalSectionDefaultTimeout;
    ULONG   DeCommitFreeBlockThreshold;
    ULONG   DeCommitTotalFreeThreshold;
    ULONG   LockPrefixTable;                // VA
    ULONG   MaximumAllocationSize;
    ULONG   VirtualMemoryThreshold;
    ULONG   ProcessHeapFlags;
    ULONG   ProcessAffinityMask;
    USHORT  CSDVersion;
    USHORT  DependentLoadFlags;
    ULONG   EditList;                       // VA
    ULONG   SecurityCookie;                 // VA
    ULONG   SEHandlerTable;                 // VA
    ULONG   SEHandlerCount;
    ULONG   GuardCFCheckFunctionPointer;    // VA
    ULONG   GuardCFDispatchFunctionPointer; // VA
    ULONG   GuardCFFunctionTable;           // VA
    ULONG   GuardCFFunctionCount;
    ULONG   GuardFlags;
    IMAGE_LOAD_CONFIG_CODE_INTEGRITY CodeIntegrity;
    ULONG   GuardAddressTakenIatEntryTable; // VA
    ULONG   GuardAddressTakenIatEntryCount;
    ULONG   GuardLongJumpTargetTable;       // VA
    ULONG   GuardLongJumpTargetCount;
    ULONG   DynamicValueRelocTable;         // VA
    ULONG   CHPEMetadataPointer;
    ULONG   GuardRFFailureRoutine;          // VA
    ULONG   GuardRFFailureRoutineFunctionPointer; // VA
    ULONG   DynamicValueRelocTableOffset;
    USHORT  DynamicValueRelocTableSection;
    USHORT  Reserved2;
    ULONG   GuardRFVerifyStackPointerFunctionPointer; // VA
    ULONG   HotPatchTableOffset;
    ULONG   Reserved3;
    ULONG   EnclaveConfigurationPointer;    // VA
    ULONG   VolatileMetadataPointer;        // VA
    ULONG   GuardEHContinuationTable;       // VA
    ULONG   GuardEHContinuationCount;
    ULONG   GuardXFGCheckFunctionPointer;    // VA
    ULONG   GuardXFGDispatchFunctionPointer; // VA
    ULONG   GuardXFGTableDispatchFunctionPointer; // VA
    ULONG   CastGuardOsDeterminedFailureMode; // VA
} IMAGE_LOAD_CONFIG_DIRECTORY32, *PIMAGE_LOAD_CONFIG_DIRECTORY32;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY64 {
    ULONG      Size;
    ULONG      TimeDateStamp;
    USHORT     MajorVersion;
    USHORT     MinorVersion;
    ULONG      GlobalFlagsClear;
    ULONG      GlobalFlagsSet;
    ULONG      CriticalSectionDefaultTimeout;
    ULONGLONG  DeCommitFreeBlockThreshold;
    ULONGLONG  DeCommitTotalFreeThreshold;
    ULONGLONG  LockPrefixTable;                // VA
    ULONGLONG  MaximumAllocationSize;
    ULONGLONG  VirtualMemoryThreshold;
    ULONGLONG  ProcessAffinityMask;
    ULONG      ProcessHeapFlags;
    USHORT     CSDVersion;
    USHORT     DependentLoadFlags;
    ULONGLONG  EditList;                       // VA
    ULONGLONG  SecurityCookie;                 // VA
    ULONGLONG  SEHandlerTable;                 // VA
    ULONGLONG  SEHandlerCount;
    ULONGLONG  GuardCFCheckFunctionPointer;    // VA
    ULONGLONG  GuardCFDispatchFunctionPointer; // VA
    ULONGLONG  GuardCFFunctionTable;           // VA
    ULONGLONG  GuardCFFunctionCount;
    ULONG      GuardFlags;
    IMAGE_LOAD_CONFIG_CODE_INTEGRITY CodeIntegrity;
    ULONGLONG  GuardAddressTakenIatEntryTable; // VA
    ULONGLONG  GuardAddressTakenIatEntryCount;
    ULONGLONG  GuardLongJumpTargetTable;       // VA
    ULONGLONG  GuardLongJumpTargetCount;
    ULONGLONG  DynamicValueRelocTable;         // VA
    ULONGLONG  CHPEMetadataPointer;            // VA
    ULONGLONG  GuardRFFailureRoutine;          // VA
    ULONGLONG  GuardRFFailureRoutineFunctionPointer; // VA
    ULONG      DynamicValueRelocTableOffset;
    USHORT     DynamicValueRelocTableSection;
    USHORT     Reserved2;
    ULONGLONG  GuardRFVerifyStackPointerFunctionPointer; // VA
    ULONG      HotPatchTableOffset;
    ULONG      Reserved3;
    ULONGLONG  EnclaveConfigurationPointer;     // VA
    ULONGLONG  VolatileMetadataPointer;         // VA
    ULONGLONG  GuardEHContinuationTable;        // VA
    ULONGLONG  GuardEHContinuationCount;
    ULONGLONG  GuardXFGCheckFunctionPointer;    // VA
    ULONGLONG  GuardXFGDispatchFunctionPointer; // VA
    ULONGLONG  GuardXFGTableDispatchFunctionPointer; // VA
    ULONGLONG  CastGuardOsDeterminedFailureMode; // VA
} IMAGE_LOAD_CONFIG_DIRECTORY64, *PIMAGE_LOAD_CONFIG_DIRECTORY64;

// end_ntoshvp
// end_winnt

typedef struct _IMAGE_CHPE_METADATA_X86 {
    ULONG  Version;
    ULONG  CHPECodeAddressRangeOffset;
    ULONG  CHPECodeAddressRangeCount;
    ULONG  WowA64ExceptionHandlerFunctionPointer;
    ULONG  WowA64DispatchCallFunctionPointer;
    ULONG  WowA64DispatchIndirectCallFunctionPointer;
    ULONG  WowA64DispatchIndirectCallCfgFunctionPointer;
    ULONG  WowA64DispatchRetFunctionPointer;
    ULONG  WowA64DispatchRetLeafFunctionPointer;
    ULONG  WowA64DispatchJumpFunctionPointer;
    ULONG  CompilerIATPointer;         // Present if Version >= 2
    ULONG  WowA64RdtscFunctionPointer; // Present if Version >= 3
} IMAGE_CHPE_METADATA_X86, *PIMAGE_CHPE_METADATA_X86;

typedef struct _IMAGE_CHPE_RANGE_ENTRY {
    union {
        ULONG StartOffset;
        struct {
            ULONG NativeCode : 1;
            ULONG AddressBits : 31;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    ULONG Length;
} IMAGE_CHPE_RANGE_ENTRY, *PIMAGE_CHPE_RANGE_ENTRY;

typedef struct _IMAGE_ARM64EC_METADATA {
    ULONG  Version;
    ULONG  CodeMap;
    ULONG  CodeMapCount;
    ULONG  CodeRangesToEntryPoints;
    ULONG  RedirectionMetadata;
    ULONG  tbd__os_arm64x_dispatch_call_no_redirect;
    ULONG  tbd__os_arm64x_dispatch_ret;
    ULONG  tbd__os_arm64x_dispatch_call;
    ULONG  tbd__os_arm64x_dispatch_icall;
    ULONG  tbd__os_arm64x_dispatch_icall_cfg;
    ULONG  AlternateEntryPoint;
    ULONG  AuxiliaryIAT;
    ULONG  CodeRangesToEntryPointsCount;
    ULONG  RedirectionMetadataCount;
    ULONG  GetX64InformationFunctionPointer;
    ULONG  SetX64InformationFunctionPointer;
    ULONG  ExtraRFETable;
    ULONG  ExtraRFETableSize;
    ULONG  __os_arm64x_dispatch_fptr;
    ULONG  AuxiliaryIATCopy;
} IMAGE_ARM64EC_METADATA;

typedef struct _IMAGE_ARM64EC_REDIRECTION_ENTRY {
    ULONG Source;
    ULONG Destination;
} IMAGE_ARM64EC_REDIRECTION_ENTRY;

typedef struct _IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT {
    ULONG StartRva;
    ULONG EndRva;
    ULONG EntryPoint;
} IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT;

#define IMAGE_DYNAMIC_RELOCATION_ARM64X     0x00000006

#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL   0
#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE      1
#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA      2

#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_2BYTES     1
#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_4BYTES     2
#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_8BYTES     3

typedef struct _IMAGE_DVRT_ARM64X_FIXUP_RECORD {
    USHORT Offset : 12;
    USHORT Type   :  2;
    USHORT Size   :  2;
} IMAGE_DVRT_ARM64X_FIXUP_RECORD, *PIMAGE_DVRT_ARM64X_FIXUP_RECORD;

typedef struct _IMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD {
    USHORT Offset : 12;
    USHORT Type   :  2;
    USHORT Sign   :  1;
    USHORT Scale  :  1;
} IMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD, *PIMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD;

// begin_winnt
// begin_ntoshvp

#ifdef _WIN64
typedef IMAGE_LOAD_CONFIG_DIRECTORY64     IMAGE_LOAD_CONFIG_DIRECTORY;
typedef PIMAGE_LOAD_CONFIG_DIRECTORY64    PIMAGE_LOAD_CONFIG_DIRECTORY;
#else
typedef IMAGE_LOAD_CONFIG_DIRECTORY32     IMAGE_LOAD_CONFIG_DIRECTORY;
typedef PIMAGE_LOAD_CONFIG_DIRECTORY32    PIMAGE_LOAD_CONFIG_DIRECTORY;
#endif

// end_ntoshvp

typedef struct _IMAGE_HOT_PATCH_INFO {
    ULONG Version;
    ULONG Size;
    ULONG SequenceNumber;
    ULONG BaseImageList;
    ULONG BaseImageCount;
    ULONG BufferOffset;             // Version 2 and later
    ULONG ExtraPatchSize;           // Version 3 and later
} IMAGE_HOT_PATCH_INFO, *PIMAGE_HOT_PATCH_INFO;

typedef struct _IMAGE_HOT_PATCH_BASE {
    ULONG SequenceNumber;
    ULONG Flags;
    ULONG OriginalTimeDateStamp;
    ULONG OriginalCheckSum;
    ULONG CodeIntegrityInfo;
    ULONG CodeIntegritySize;
    ULONG PatchTable;
    ULONG BufferOffset;             // Version 2 and later
} IMAGE_HOT_PATCH_BASE, *PIMAGE_HOT_PATCH_BASE;

typedef struct _IMAGE_HOT_PATCH_HASHES {
    UCHAR SHA256[32];
    UCHAR SHA1[20];
} IMAGE_HOT_PATCH_HASHES, *PIMAGE_HOT_PATCH_HASHES;

#define IMAGE_HOT_PATCH_BASE_OBLIGATORY     0x00000001
#define IMAGE_HOT_PATCH_BASE_CAN_ROLL_BACK  0x00000002

#define IMAGE_HOT_PATCH_CHUNK_INVERSE       0x80000000
#define IMAGE_HOT_PATCH_CHUNK_OBLIGATORY    0x40000000
#define IMAGE_HOT_PATCH_CHUNK_RESERVED      0x3FF03000
#define IMAGE_HOT_PATCH_CHUNK_TYPE          0x000FC000
#define IMAGE_HOT_PATCH_CHUNK_SOURCE_RVA    0x00008000
#define IMAGE_HOT_PATCH_CHUNK_TARGET_RVA    0x00004000
#define IMAGE_HOT_PATCH_CHUNK_SIZE          0x00000FFF

#define IMAGE_HOT_PATCH_NONE                0x00000000
#define IMAGE_HOT_PATCH_FUNCTION            0x0001C000
#define IMAGE_HOT_PATCH_ABSOLUTE            0x0002C000
#define IMAGE_HOT_PATCH_REL32               0x0003C000
#define IMAGE_HOT_PATCH_CALL_TARGET         0x00044000
#define IMAGE_HOT_PATCH_INDIRECT            0x0005C000
#define IMAGE_HOT_PATCH_NO_CALL_TARGET      0x00064000
#define IMAGE_HOT_PATCH_DYNAMIC_VALUE       0x00078000

#define IMAGE_GUARD_CF_INSTRUMENTED                    0x00000100 // Module performs control flow integrity checks using system-supplied support
#define IMAGE_GUARD_CFW_INSTRUMENTED                   0x00000200 // Module performs control flow and write integrity checks
#define IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT          0x00000400 // Module contains valid control flow target metadata
#define IMAGE_GUARD_SECURITY_COOKIE_UNUSED             0x00000800 // Module does not make use of the /GS security cookie
#define IMAGE_GUARD_PROTECT_DELAYLOAD_IAT              0x00001000 // Module supports read only delay load IAT
#define IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION   0x00002000 // Delayload import table in its own .didat section (with nothing else in it) that can be freely reprotected
#define IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT 0x00004000 // Module contains suppressed export information. This also infers that the address taken
                                                                  // taken IAT table is also present in the load config.
#define IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION       0x00008000 // Module enables suppression of exports
#define IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT          0x00010000 // Module contains longjmp target information
#define IMAGE_GUARD_RF_INSTRUMENTED                    0x00020000 // Module contains return flow instrumentation and metadata
#define IMAGE_GUARD_RF_ENABLE                          0x00040000 // Module requests that the OS enable return flow protection
#define IMAGE_GUARD_RF_STRICT                          0x00080000 // Module requests that the OS enable return flow protection in strict mode
#define IMAGE_GUARD_RETPOLINE_PRESENT                  0x00100000 // Module was built with retpoline support
// DO_NOT_USE                                          0x00200000 // Was EHCont flag on VB (20H1)
#define IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT      0x00400000 // Module contains EH continuation target information
#define IMAGE_GUARD_XFG_ENABLED                        0x00800000 // Module was built with xfg

#define IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK        0xF0000000 // Stride of Guard CF function table encoded in these bits (additional count of bytes per element)
#define IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT       28         // Shift to right-justify Guard CF function table stride

//
// GFIDS table entry flags.
//

#define IMAGE_GUARD_FLAG_FID_SUPPRESSED               0x01       // The containing GFID entry is suppressed
#define IMAGE_GUARD_FLAG_EXPORT_SUPPRESSED            0x02       // The containing GFID entry is export suppressed
#define IMAGE_GUARD_FLAG_FID_LANGEXCPTHANDLER         0x04
#define IMAGE_GUARD_FLAG_FID_XFG                      0x08

//
// WIN CE Exception table format
//

//
// Function table entry format.  Function table is pointed to by the
// IMAGE_DIRECTORY_ENTRY_EXCEPTION directory entry.
//

typedef struct _IMAGE_CE_RUNTIME_FUNCTION_ENTRY {
    ULONG FuncStart;
    ULONG PrologLen : 8;
    ULONG FuncLen : 22;
    ULONG ThirtyTwoBit : 1;
    ULONG ExceptionFlag : 1;
} IMAGE_CE_RUNTIME_FUNCTION_ENTRY, * PIMAGE_CE_RUNTIME_FUNCTION_ENTRY;

typedef struct _IMAGE_ARM_RUNTIME_FUNCTION_ENTRY {
    ULONG BeginAddress;
    union {
        ULONG UnwindData;
        struct {
            ULONG Flag : 2;
            ULONG FunctionLength : 11;
            ULONG Ret : 2;
            ULONG H : 1;
            ULONG Reg : 3;
            ULONG R : 1;
            ULONG L : 1;
            ULONG C : 1;
            ULONG StackAdjust : 10;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} IMAGE_ARM_RUNTIME_FUNCTION_ENTRY, * PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY;

typedef enum ARM64_FNPDATA_FLAGS {
    PdataRefToFullXdata = 0,
    PdataPackedUnwindFunction = 1,
    PdataPackedUnwindFragment = 2,
} ARM64_FNPDATA_FLAGS;

typedef enum ARM64_FNPDATA_CR {
    PdataCrUnchained = 0,
    PdataCrUnchainedSavedLr = 1,
    PdataCrChainedWithPac = 2,
    PdataCrChained = 3,
} ARM64_FNPDATA_CR;

typedef struct _IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY {
    ULONG BeginAddress;
    union {
        ULONG UnwindData;
        struct {
            ULONG Flag : 2;
            ULONG FunctionLength : 11;
            ULONG RegF : 3;
            ULONG RegI : 4;
            ULONG H : 1;
            ULONG CR : 2;
            ULONG FrameSize : 9;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY, * PIMAGE_ARM64_RUNTIME_FUNCTION_ENTRY;

typedef union IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA {
    ULONG HeaderData;
    struct {
        ULONG FunctionLength : 18;      // in words (2 bytes)
        ULONG Version : 2;
        ULONG ExceptionDataPresent : 1;
        ULONG EpilogInHeader : 1;
        ULONG EpilogCount : 5;          // number of epilogs or byte index of the first unwind code for the one only epilog
        ULONG CodeWords : 5;            // number of dwords with unwind codes
    };
} IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA;

typedef struct _IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY {
    ULONGLONG BeginAddress;
    ULONGLONG EndAddress;
    ULONGLONG ExceptionHandler;
    ULONGLONG HandlerData;
    ULONGLONG PrologEndAddress;
} IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY, *PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY;

typedef struct _IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG ExceptionHandler;
    ULONG HandlerData;
    ULONG PrologEndAddress;
} IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY, *PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY;

typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY {
    ULONG BeginAddress;
    ULONG EndAddress;
    union {
        ULONG UnwindInfoAddress;
        ULONG UnwindData;
    } DUMMYUNIONNAME;
} _IMAGE_RUNTIME_FUNCTION_ENTRY, *_PIMAGE_RUNTIME_FUNCTION_ENTRY;

typedef  _IMAGE_RUNTIME_FUNCTION_ENTRY  IMAGE_IA64_RUNTIME_FUNCTION_ENTRY;
typedef _PIMAGE_RUNTIME_FUNCTION_ENTRY PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY;

typedef  _IMAGE_RUNTIME_FUNCTION_ENTRY  IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY;
typedef _PIMAGE_RUNTIME_FUNCTION_ENTRY PIMAGE_AMD64_RUNTIME_FUNCTION_ENTRY;

#if defined(_AXP64_)

typedef  IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  IMAGE_AXP64_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY PIMAGE_AXP64_RUNTIME_FUNCTION_ENTRY;
typedef  IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#elif defined(_ALPHA_)

typedef  IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#elif defined(_ARM64_)

typedef  IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ARM64_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#elif defined(_ARM_)

typedef  IMAGE_ARM_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#else

typedef  _IMAGE_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef _PIMAGE_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#endif

//
// Sofware enclave information
//

#define IMAGE_ENCLAVE_LONG_ID_LENGTH    ENCLAVE_LONG_ID_LENGTH
#define IMAGE_ENCLAVE_SHORT_ID_LENGTH   ENCLAVE_SHORT_ID_LENGTH

typedef struct _IMAGE_ENCLAVE_CONFIG32 {
    ULONG Size;
    ULONG MinimumRequiredConfigSize;
    ULONG PolicyFlags;
    ULONG NumberOfImports;
    ULONG ImportList;
    ULONG ImportEntrySize;
    UCHAR FamilyID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    UCHAR ImageID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    ULONG ImageVersion;
    ULONG SecurityVersion;
    ULONG EnclaveSize;
    ULONG NumberOfThreads;
    ULONG EnclaveFlags;
} IMAGE_ENCLAVE_CONFIG32, *PIMAGE_ENCLAVE_CONFIG32;

typedef struct _IMAGE_ENCLAVE_CONFIG64 {
    ULONG Size;
    ULONG MinimumRequiredConfigSize;
    ULONG PolicyFlags;
    ULONG NumberOfImports;
    ULONG ImportList;
    ULONG ImportEntrySize;
    UCHAR FamilyID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    UCHAR ImageID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    ULONG ImageVersion;
    ULONG SecurityVersion;
    ULONGLONG EnclaveSize;
    ULONG NumberOfThreads;
    ULONG EnclaveFlags;
} IMAGE_ENCLAVE_CONFIG64, *PIMAGE_ENCLAVE_CONFIG64;

#ifdef _WIN64
typedef IMAGE_ENCLAVE_CONFIG64          IMAGE_ENCLAVE_CONFIG;
typedef PIMAGE_ENCLAVE_CONFIG64         PIMAGE_ENCLAVE_CONFIG;
#else
typedef IMAGE_ENCLAVE_CONFIG32          IMAGE_ENCLAVE_CONFIG;
typedef PIMAGE_ENCLAVE_CONFIG32         PIMAGE_ENCLAVE_CONFIG;
#endif

#define IMAGE_ENCLAVE_MINIMUM_CONFIG_SIZE   FIELD_OFFSET(IMAGE_ENCLAVE_CONFIG, EnclaveFlags)

#define IMAGE_ENCLAVE_POLICY_DEBUGGABLE     0x00000001

#define IMAGE_ENCLAVE_FLAG_PRIMARY_IMAGE    0x00000001

typedef struct _IMAGE_ENCLAVE_IMPORT {
    ULONG MatchType;
    ULONG MinimumSecurityVersion;
    UCHAR UniqueOrAuthorID[IMAGE_ENCLAVE_LONG_ID_LENGTH];
    UCHAR FamilyID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    UCHAR ImageID[IMAGE_ENCLAVE_SHORT_ID_LENGTH];
    ULONG ImportName;
    ULONG Reserved;
} IMAGE_ENCLAVE_IMPORT, *PIMAGE_ENCLAVE_IMPORT;

#define IMAGE_ENCLAVE_IMPORT_MATCH_NONE             0x00000000
#define IMAGE_ENCLAVE_IMPORT_MATCH_UNIQUE_ID        0x00000001
#define IMAGE_ENCLAVE_IMPORT_MATCH_AUTHOR_ID        0x00000002
#define IMAGE_ENCLAVE_IMPORT_MATCH_FAMILY_ID        0x00000003
#define IMAGE_ENCLAVE_IMPORT_MATCH_IMAGE_ID         0x00000004

//
// Debug Format
//

typedef struct _IMAGE_DEBUG_DIRECTORY {
    ULONG   Characteristics;
    ULONG   TimeDateStamp;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    ULONG   Type;
    ULONG   SizeOfData;
    ULONG   AddressOfRawData;
    ULONG   PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN                0
#define IMAGE_DEBUG_TYPE_COFF                   1
#define IMAGE_DEBUG_TYPE_CODEVIEW               2
#define IMAGE_DEBUG_TYPE_FPO                    3
#define IMAGE_DEBUG_TYPE_MISC                   4
#define IMAGE_DEBUG_TYPE_EXCEPTION              5
#define IMAGE_DEBUG_TYPE_FIXUP                  6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC            7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC          8
#define IMAGE_DEBUG_TYPE_BORLAND                9
#define IMAGE_DEBUG_TYPE_RESERVED10             10
#define IMAGE_DEBUG_TYPE_CLSID                  11
#define IMAGE_DEBUG_TYPE_VC_FEATURE             12
#define IMAGE_DEBUG_TYPE_POGO                   13
#define IMAGE_DEBUG_TYPE_ILTCG                  14
#define IMAGE_DEBUG_TYPE_MPX                    15
#define IMAGE_DEBUG_TYPE_REPRO                  16
#define IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS  20

#define IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT                                  0x01
#define IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT_STRICT_MODE                      0x02
#define IMAGE_DLLCHARACTERISTICS_EX_CET_SET_CONTEXT_IP_VALIDATION_RELAXED_MODE  0x04
#define IMAGE_DLLCHARACTERISTICS_EX_CET_DYNAMIC_APIS_ALLOW_IN_PROC              0x08
#define IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_1                              0x10  // Reserved for CET policy *downgrade* only!
#define IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_2                              0x20  // Reserved for CET policy *downgrade* only!

// end_winnt

// IMAGE_DEBUG_TYPE values > 0x7FFFFFFF are reserved for BBT

// begin_winnt

typedef struct _IMAGE_COFF_SYMBOLS_HEADER {
    ULONG   NumberOfSymbols;
    ULONG   LvaToFirstSymbol;
    ULONG   NumberOfLinenumbers;
    ULONG   LvaToFirstLinenumber;
    ULONG   RvaToFirstByteOfCode;
    ULONG   RvaToLastByteOfCode;
    ULONG   RvaToFirstByteOfData;
    ULONG   RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER, *PIMAGE_COFF_SYMBOLS_HEADER;

#define FRAME_FPO       0
#define FRAME_TRAP      1
#define FRAME_TSS       2
#define FRAME_NONFPO    3

typedef struct _FPO_DATA {
    ULONG       ulOffStart;             // offset 1st byte of function code
    ULONG       cbProcSize;             // # bytes in function
    ULONG       cdwLocals;              // # bytes in locals/4
    USHORT      cdwParams;              // # bytes in params/4
    USHORT      cbProlog : 8;           // # bytes in prolog
    USHORT      cbRegs   : 3;           // # regs saved
    USHORT      fHasSEH  : 1;           // TRUE if SEH in func
    USHORT      fUseBP   : 1;           // TRUE if EBP has been allocated
    USHORT      reserved : 1;           // reserved for future use
    USHORT      cbFrame  : 2;           // frame type
} FPO_DATA, *PFPO_DATA;
#define SIZEOF_RFPO_DATA 16

#define IMAGE_DEBUG_MISC_EXENAME    1

typedef struct _IMAGE_DEBUG_MISC {
    ULONG       DataType;               // type of misc data, see defines
    ULONG       Length;                 // total length of record, rounded to four
                                        // byte multiple.
    BOOLEAN     Unicode;                // TRUE if data is unicode string
    UCHAR       Reserved[ 3 ];
    UCHAR       Data[ 1 ];              // Actual data
} IMAGE_DEBUG_MISC, *PIMAGE_DEBUG_MISC;

//
// Function table extracted from MIPS/ALPHA/IA64 images.  Does not contain
// information needed only for runtime support.  Just those fields for
// each entry needed by a debugger.
//

typedef struct _IMAGE_FUNCTION_ENTRY {
    ULONG   StartingAddress;
    ULONG   EndingAddress;
    ULONG   EndOfPrologue;
} IMAGE_FUNCTION_ENTRY, *PIMAGE_FUNCTION_ENTRY;

typedef struct _IMAGE_FUNCTION_ENTRY64 {
    ULONGLONG   StartingAddress;
    ULONGLONG   EndingAddress;
    union {
        ULONGLONG   EndOfPrologue;
        ULONGLONG   UnwindInfoAddress;
    } DUMMYUNIONNAME;
} IMAGE_FUNCTION_ENTRY64, *PIMAGE_FUNCTION_ENTRY64;

//
// Debugging information can be stripped from an image file and placed
// in a separate .DBG file, whose file name part is the same as the
// image file name part (e.g. symbols for CMD.EXE could be stripped
// and placed in CMD.DBG).  This is indicated by the IMAGE_FILE_DEBUG_STRIPPED
// flag in the Characteristics field of the file header.  The beginning of
// the .DBG file contains the following structure which captures certain
// information from the image file.  This allows a debug to proceed even if
// the original image file is not accessable.  This header is followed by
// zero of more IMAGE_SECTION_HEADER structures, followed by zero or more
// IMAGE_DEBUG_DIRECTORY structures.  The latter structures and those in
// the image file contain file offsets relative to the beginning of the
// .DBG file.
//
// If symbols have been stripped from an image, the IMAGE_DEBUG_MISC structure
// is left in the image file, but not mapped.  This allows a debugger to
// compute the name of the .DBG file, from the name of the image in the
// IMAGE_DEBUG_MISC structure.
//

typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
    USHORT      Signature;
    USHORT      Flags;
    USHORT      Machine;
    USHORT      Characteristics;
    ULONG       TimeDateStamp;
    ULONG       CheckSum;
    ULONG       ImageBase;
    ULONG       SizeOfImage;
    ULONG       NumberOfSections;
    ULONG       ExportedNamesSize;
    ULONG       DebugDirectorySize;
    ULONG       SectionAlignment;
    ULONG       Reserved[2];
} IMAGE_SEPARATE_DEBUG_HEADER, *PIMAGE_SEPARATE_DEBUG_HEADER;

// begin_ntoshvp

typedef struct _NON_PAGED_DEBUG_INFO {
    USHORT      Signature;
    USHORT      Flags;
    ULONG       Size;
    USHORT      Machine;
    USHORT      Characteristics;
    ULONG       TimeDateStamp;
    ULONG       CheckSum;
    ULONG       SizeOfImage;
    ULONGLONG   ImageBase;
    //DebugDirectorySize
    //IMAGE_DEBUG_DIRECTORY
} NON_PAGED_DEBUG_INFO, *PNON_PAGED_DEBUG_INFO;

// end_ntoshvp

#ifndef _MAC
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944
#define NON_PAGED_DEBUG_SIGNATURE      0x494E
#else
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4449  // DI
#define NON_PAGED_DEBUG_SIGNATURE      0x4E49  // NI
#endif

#define IMAGE_SEPARATE_DEBUG_FLAGS_MASK 0x8000
#define IMAGE_SEPARATE_DEBUG_MISMATCH   0x8000  // when DBG was updated, the
                                                // old checksum didn't match.

//
//  The .arch section is made up of headers, each describing an amask position/value
//  pointing to an array of IMAGE_ARCHITECTURE_ENTRY's.  Each "array" (both the header
//  and entry arrays) are terminiated by a quadword of 0xffffffffL.
//
//  NOTE: There may be quadwords of 0 sprinkled around and must be skipped.
//

typedef struct _ImageArchitectureHeader {
    unsigned int AmaskValue: 1;                 // 1 -> code section depends on mask bit
                                                // 0 -> new instruction depends on mask bit
    int :7;                                     // MBZ
    unsigned int AmaskShift: 8;                 // Amask bit in question for this fixup
    int :16;                                    // MBZ
    ULONG FirstEntryRVA;                        // RVA into .arch section to array of ARCHITECTURE_ENTRY's
} IMAGE_ARCHITECTURE_HEADER, *PIMAGE_ARCHITECTURE_HEADER;

typedef struct _ImageArchitectureEntry {
    ULONG FixupInstRVA;                         // RVA of instruction to fixup
    ULONG NewInst;                              // fixup instruction (see alphaops.h)
} IMAGE_ARCHITECTURE_ENTRY, *PIMAGE_ARCHITECTURE_ENTRY;

#include "poppack.h"                // Back to the initial value

// The following structure defines the new import object.  Note the values of the first two fields,
// which must be set as stated in order to differentiate old and new import members.
// Following this structure, the linker emits two null-terminated strings used to recreate the
// import at the time of use.  The first string is the import's name, the second is the dll's name.

#define IMPORT_OBJECT_HDR_SIG2  0xffff

typedef struct IMPORT_OBJECT_HEADER {
    USHORT  Sig1;                       // Must be IMAGE_FILE_MACHINE_UNKNOWN
    USHORT  Sig2;                       // Must be IMPORT_OBJECT_HDR_SIG2.
    USHORT  Version;
    USHORT  Machine;
    ULONG   TimeDateStamp;              // Time/date stamp
    ULONG   SizeOfData;                 // particularly useful for incremental links

    union {
        USHORT  Ordinal;                // if grf & IMPORT_OBJECT_ORDINAL
        USHORT  Hint;
    } DUMMYUNIONNAME;

    USHORT  Type : 2;                   // IMPORT_TYPE
    USHORT  NameType : 3;               // IMPORT_NAME_TYPE
    USHORT  Reserved : 11;              // Reserved. Must be zero.
} IMPORT_OBJECT_HEADER;

typedef enum IMPORT_OBJECT_TYPE
{
    IMPORT_OBJECT_CODE = 0,
    IMPORT_OBJECT_DATA = 1,
    IMPORT_OBJECT_CONST = 2,
} IMPORT_OBJECT_TYPE;

typedef enum IMPORT_OBJECT_NAME_TYPE
{
    IMPORT_OBJECT_ORDINAL = 0,          // Import by ordinal
    IMPORT_OBJECT_NAME = 1,             // Import name == public symbol name.
    IMPORT_OBJECT_NAME_NO_PREFIX = 2,   // Import name == public symbol name skipping leading ?, @, or optionally _.
    IMPORT_OBJECT_NAME_UNDECORATE = 3,  // Import name == public symbol name skipping leading ?, @, or optionally _
                                        //  and truncating at first @.
    IMPORT_OBJECT_NAME_EXPORTAS = 4,    // Import name == a name is explicitly provided after the DLL name.
} IMPORT_OBJECT_NAME_TYPE;

// end_winnt

// The structure is used by the NT loader for clr URT support.  It
// is a duplicate of the definition in corhdr.h.

// begin_winnt

#ifndef __IMAGE_COR20_HEADER_DEFINED__
#define __IMAGE_COR20_HEADER_DEFINED__

typedef enum ReplacesCorHdrNumericDefines
{
// COM+ Header entry point flags.
    COMIMAGE_FLAGS_ILONLY               =0x00000001,
    COMIMAGE_FLAGS_32BITREQUIRED        =0x00000002,
    COMIMAGE_FLAGS_IL_LIBRARY           =0x00000004,
    COMIMAGE_FLAGS_STRONGNAMESIGNED     =0x00000008,
    COMIMAGE_FLAGS_NATIVE_ENTRYPOINT    =0x00000010,
    COMIMAGE_FLAGS_TRACKDEBUGDATA       =0x00010000,
    COMIMAGE_FLAGS_32BITPREFERRED       =0x00020000,

// Version flags for image.
    COR_VERSION_MAJOR_V2                =2,
    COR_VERSION_MAJOR                   =COR_VERSION_MAJOR_V2,
    COR_VERSION_MINOR                   =5,
    COR_DELETED_NAME_LENGTH             =8,
    COR_VTABLEGAP_NAME_LENGTH           =8,

// Maximum size of a NativeType descriptor.
    NATIVE_TYPE_MAX_CB                  =1,
    COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE=0xFF,

// #defines for the MIH FLAGS
    IMAGE_COR_MIH_METHODRVA             =0x01,
    IMAGE_COR_MIH_EHRVA                 =0x02,
    IMAGE_COR_MIH_BASICBLOCK            =0x08,

// V-table constants
    COR_VTABLE_32BIT                    =0x01,          // V-table slots are 32-bits in size.
    COR_VTABLE_64BIT                    =0x02,          // V-table slots are 64-bits in size.
    COR_VTABLE_FROM_UNMANAGED           =0x04,          // If set, transition from unmanaged.
    COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN  =0x08,  // If set, transition from unmanaged with keeping the current appdomain.
    COR_VTABLE_CALL_MOST_DERIVED        =0x10,          // Call most derived method described by

// EATJ constants
    IMAGE_COR_EATJ_THUNK_SIZE           =32,            // Size of a jump thunk reserved range.

// Max name lengths
    //@todo: Change to unlimited name lengths.
    MAX_CLASS_NAME                      =1024,
    MAX_PACKAGE_NAME                    =1024,
} ReplacesCorHdrNumericDefines;

// CLR 2.0 header structure.
typedef struct IMAGE_COR20_HEADER
{
    // Header versioning
    ULONG                   cb;
    USHORT                  MajorRuntimeVersion;
    USHORT                  MinorRuntimeVersion;

    // Symbol table and startup information
    IMAGE_DATA_DIRECTORY    MetaData;
    ULONG                   Flags;

    // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is not set, EntryPointToken represents a managed entrypoint.
    // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is set, EntryPointRVA represents an RVA to a native entrypoint.
    union {
        ULONG               EntryPointToken;
        ULONG               EntryPointRVA;
    } DUMMYUNIONNAME;

    // Binding information
    IMAGE_DATA_DIRECTORY    Resources;
    IMAGE_DATA_DIRECTORY    StrongNameSignature;

    // Regular fixup and binding information
    IMAGE_DATA_DIRECTORY    CodeManagerTable;
    IMAGE_DATA_DIRECTORY    VTableFixups;
    IMAGE_DATA_DIRECTORY    ExportAddressTableJumps;

    // Precompiled image info (internal use only - set to zero)
    IMAGE_DATA_DIRECTORY    ManagedNativeHeader;

} IMAGE_COR20_HEADER, *PIMAGE_COR20_HEADER;

#endif // __IMAGE_COR20_HEADER_DEFINED__

//
// End Image Format
//

// end_winnt

typedef IMAGE_OS2_HEADER UNALIGNED * PUIMAGE_OS2_HEADER;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED CONST *PCIMAGE_IMPORT_DESCRIPTOR;
typedef CONST IMAGE_BOUND_IMPORT_DESCRIPTOR *PCIMAGE_BOUND_IMPORT_DESCRIPTOR;
typedef CONST IMAGE_BOUND_FORWARDER_REF     *PCIMAGE_BOUND_FORWARDER_REF;
typedef CONST IMAGE_IMPORT_BY_NAME          *PCIMAGE_IMPORT_BY_NAME;
typedef CONST IMAGE_THUNK_DATA              *PCIMAGE_THUNK_DATA;
typedef CONST IMAGE_THUNK_DATA32            *PCIMAGE_THUNK_DATA32;
typedef CONST IMAGE_THUNK_DATA64            *PCIMAGE_THUNK_DATA64;
typedef CONST IMAGE_TLS_DIRECTORY           *PCIMAGE_TLS_DIRECTORY;
typedef CONST IMAGE_TLS_DIRECTORY32         *PCIMAGE_TLS_DIRECTORY32;
typedef CONST IMAGE_TLS_DIRECTORY64         *PCIMAGE_TLS_DIRECTORY64;
typedef CONST IMAGE_EXPORT_DIRECTORY        *PCIMAGE_EXPORT_DIRECTORY;
typedef CONST IMAGE_SECTION_HEADER          *PCIMAGE_SECTION_HEADER;

#define PATCH_MAIN_CALLOUT_FUNCTION_NAME "__PatchMainCallout__"

#define PATCH_IMAGE_PHASE_1        0
#define PATCH_IMAGE_PHASE_2        1
#define PATCH_IMAGE_PHASE_2_FAILED 2

#define PATCH_MAIN_CALLOUT_PARAMS_VERSION 1

typedef struct _PATCH_MAIN_CALLOUT_PARAMS
{

    ULONG Version;

    //
    // Phase of the callout
    //

    ULONG PatchPhase;

    //
    // Flags field for future expansion
    //

    ULONG Flags;

} PATCH_MAIN_CALLOUT_PARAMS, *PPATCH_MAIN_CALLOUT_PARAMS;

typedef NTSTATUS (*PATCH_MAIN_CALLOUT_FUNCTION)(PPATCH_MAIN_CALLOUT_PARAMS, ULONG);

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#endif // _NTIMAGE_
