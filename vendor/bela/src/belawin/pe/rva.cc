//
#include "internal.hpp"

namespace bela::pe {
// https://docs.microsoft.com/en-us/windows/desktop/Debug/pe-format
// PE32+ executable (console) x86-64, for MS Windows
// PE32 executable (DLL) (console) Intel 80386 Mono/.Net assembly, for MS
// Windows PE32 executable (console) Intel 80386, for MS Windows file command
// not support check arm and arm64
// Not depend DebHelp.dll
typedef enum ReplacesGeneralNumericDefines {
// Directory entry macro for CLR data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
  IMAGE_DIRECTORY_ENTRY_COMHEADER = 14,
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER
} ReplacesGeneralNumericDefines;
#define STORAGE_MAGIC_SIG 0x424A5342 // BSJB

#pragma pack(1)
struct STORAGESIGNATURE {
  ULONG Signature;     // Magic signature for physical metadata : 0x424A5342.
  USHORT MajorVersion; // Major version, 1 (ignore on read)
  USHORT MinorVersion; // Minor version, 0 (ignore on read)
  ULONG ExtraData;     // offset to next structure of information
  ULONG Length;        // Length of version string in bytes
};
#pragma pack()
// LE endian
[[maybe_unused]] inline PVOID belarva(PVOID m, PVOID b) {
  return reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(b) + reinterpret_cast<ULONG_PTR>(m));
}

PIMAGE_SECTION_HEADER
BelaImageRvaToSection(PIMAGE_NT_HEADERS nh, PVOID BaseAddress, ULONG rva) {
  (void)BaseAddress;
  ULONG count = bela::swaple(nh->FileHeader.NumberOfSections);
  PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nh);
  ULONG va = 0;
  while (count-- != 0) {
    va = bela::swaple(section->VirtualAddress);
    if ((va <= rva) && (rva < va + bela::swaple(section->SizeOfRawData))) {
      return section;
    }
    section++;
  }
  return nullptr;
}

// like RtlImageRvaToVa
PVOID
BelaImageRvaToVa(PIMAGE_NT_HEADERS nh, PVOID BaseAddress, ULONG rva, PIMAGE_SECTION_HEADER *sh) {
  PIMAGE_SECTION_HEADER section = nullptr;
  if (sh != nullptr) {
    section = *sh;
  }
  if ((section == nullptr) || (rva < bela::swaple(section->VirtualAddress)) ||
      (rva >= bela::swaple(section->VirtualAddress) + bela::swaple(section->SizeOfRawData))) {
    section = BelaImageRvaToSection(nh, BaseAddress, rva);
    if (section == nullptr) {
      return nullptr;
    }
    if (sh) {
      *sh = section;
    }
  }
  auto va = reinterpret_cast<ULONG_PTR>(BaseAddress) + rva +
            static_cast<ULONG_PTR>(bela::swaple(section->PointerToRawData)) -
            static_cast<ULONG_PTR>(bela::swaple(section->VirtualAddress));
  return reinterpret_cast<PVOID>(va);
}

} // namespace bela::pe