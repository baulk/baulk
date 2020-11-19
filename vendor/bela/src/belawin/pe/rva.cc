//
#include "internal.hpp"

namespace bela::pe {
// https://docs.microsoft.com/en-us/windows/desktop/Debug/pe-format
// PE32+ executable (console) x86-64, for MS Windows
// PE32 executable (DLL) (console) Intel 80386 Mono/.Net assembly, for MS
// Windows PE32 executable (console) Intel 80386, for MS Windows file command
// not support check arm and arm64
// Not depend DebHelp.dll
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