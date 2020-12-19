//
#include "hazelinc.hpp"

namespace hazel::internal {
static constexpr const uint8_t ElfMagic[] = {0x7f, 'E', 'L', 'F'};

constexpr auto FF = (std::numeric_limits<uint8_t>::max)();

// The PE signature bytes that follows the DOS stub header.
static constexpr const uint8_t PEMagic[] = {'P', 'E', '\0', '\0'};
static constexpr const uint8_t BigObjMagic[] = {
    '\xc7', '\xa1', '\xba', '\xd1', '\xee', '\xba', '\xa9', '\x4b',
    '\xaf', '\x20', '\xfa', '\xf6', '\x6a', '\xa4', '\xdc', '\xb8',
};
static constexpr const uint8_t ClGlObjMagic[] = {
    '\x38', '\xfe', '\xb3', '\x0c', '\xa5', '\xd9', '\xab', '\x4d',
    '\xac', '\x9b', '\xd6', '\xb6', '\x22', '\x26', '\x53', '\xc2',
};
// The signature bytes that start a .res file.
static constexpr const uint8_t WinResMagic[] = {
    '\x00', '\x00', '\x00', '\x00', '\x20', '\x00', '\x00', '\x00', FF, FF, '\x00', '\x00', FF, FF, '\x00', '\x00',
};
static constexpr const uint8_t debMagic[] = {0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A, 0x64, 0x65, 0x62,
                                             0x69, 0x61, 0x6E, 0x2D, 0x62, 0x69, 0x6E, 0x61, 0x72, 0x79};
struct BigObjHeader {
  enum : uint16_t { MinBigObjectVersion = 2 };

  uint16_t Sig1; ///< Must be IMAGE_FILE_MACHINE_UNKNOWN (0).
  uint16_t Sig2; ///< Must be 0xFFFF.
  uint16_t Version;
  uint16_t Machine;
  uint32_t TimeDateStamp;
  uint8_t UUID[16];
  uint32_t unused1;
  uint32_t unused2;
  uint32_t unused3;
  uint32_t unused4;
  uint32_t NumberOfSections;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
};

constexpr std::string_view wasmobj{"\0asm", 4};
constexpr std::string_view irobj{"\xDE\xC0\x17\x0B", 4};
constexpr std::string_view irobj2{"\xBC\xC0\xDE", 3};
constexpr std::string_view bobj{"\0\0\xFF\xFF", 4};

struct mach_header {
  uint32_t magic;      /* mach magic number identifier */
  uint32_t cputype;    /* cpu specifier */
  uint32_t cpusubtype; /* machine specifier */
  uint32_t filetype;   /* type of file */
  uint32_t ncmds;      /* number of load commands */
  uint32_t sizeofcmds; /* the size of all the load commands */
  uint32_t flags;      /* flags */
};

/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
  uint32_t magic;      /* mach magic number identifier */
  uint32_t cputype;    /* cpu specifier */
  uint32_t cpusubtype; /* machine specifier */
  uint32_t filetype;   /* type of file */
  uint32_t ncmds;      /* number of load commands */
  uint32_t sizeofcmds; /* the size of all the load commands */
  uint32_t flags;      /* flags */
  uint32_t reserved;   /* reserved */
};

status_t LookupExecutableFile(bela::MemView mv, hazel_result &hr) {
  if (mv.size() < 4) {
    return None;
  }
  switch (mv[0]) {
  case 0x00:
    if (mv.StartsWith(bobj)) {
      size_t minsize = offsetof(BigObjHeader, UUID) + sizeof(BigObjMagic);
      if (mv.size() < minsize) {
        hr.assign(types::coff_import_library, L"COFF import library");
        return Found;
      }
      const char *start = reinterpret_cast<const char *>(mv.data()) + offsetof(BigObjHeader, UUID);
      if (memcmp(start, BigObjMagic, sizeof(BigObjMagic)) == 0) {
        hr.assign(types::coff_object, L"COFF object");
        return Found;
      }
      if (memcmp(start, ClGlObjMagic, sizeof(ClGlObjMagic)) == 0) {
        hr.assign(types::coff_cl_gl_object, L"Microsoft cl.exe's intermediate code file");
        return Found;
      }
      hr.assign(types::coff_import_library, L"COFF import library");
      return Found;
    }
    if (mv.size() >= sizeof(WinResMagic) && memcmp(mv.data(), WinResMagic, sizeof(WinResMagic)) == 0) {
      hr.assign(types::windows_resource, L"Windows compiled resource file (.res)");
      return Found;
    }
    // if (mv[1] == 0) {
    //   hr.assign(L"COFF object", types::coff_object);
    //   return Found;
    // }
    if (mv.StartsWith(wasmobj)) {
      hr.assign(types::wasm_object, L"WebAssembly Object file");
      return Found;
    }
    break;
  case 0xDE:
    if (mv.StartsWith(irobj)) {
      hr.assign(types::bitcode, L"LLVM IR bitcode");
      return Found;
    }
    break;
  case 'B':
    if (mv.StartsWith(irobj2)) {
      hr.assign(types::bitcode, L"LLVM IR bitcode");
      return Found;
    }
    break;
  case '!': // .a
    if (mv.StartsWith("!<arch>\n") && !mv.StartsWith(debMagic) || mv.StartsWith("!<thin>\n")) {
      // Skip DEB package
      hr.assign(types::archive, L"ar style archive file");
      return Found;
    }
    break;
  case '\177': // ELF
    if (mv.StartsWith(ElfMagic) && mv.size() >= 18) {
      bool Data2MSB = (mv[5] == 2);
      unsigned high = Data2MSB ? 16 : 17;
      unsigned low = Data2MSB ? 17 : 16;
      if (mv[high] == 0) {
        switch (mv[low]) {
        default:
          break;
        case 1:
          hr.assign(types::elf_relocatable, L"ELF relocatable object file");
          return Found;
        case 2:
          hr.assign(types::elf_executable, L"ELF executable image");
          return Found;
        case 3:
          hr.assign(types::elf_shared_object, L"ELF dynamically linked shared lib");
          return Found;
        case 4:
          hr.assign(types::elf_core, L"ELF core image");
          return Found;
        }
      }
      hr.assign(types::elf, L"ELF unknown type");
      return Found;
    }
    break;
  case 0xCA:
    if (mv.StartsWith("\xCA\xFE\xBA\xBE") || mv.StartsWith("\xCA\xFE\xBA\xBF")) {
      if (mv.size() >= 8 && mv[7] < 43) {
        hr.assign(types::macho_universal_binary, L"Mach-O universal binary");
        return Found;
      }
    }
    break;
  case 0xFE:
  case 0xCE:
  case 0xCF: {
    uint16_t type = 0;
    if (mv.StartsWith("\xFE\xED\xFA\xCE") || mv.StartsWith("\xFE\xED\xFA\xCF")) {
      /* Native endian */
      size_t minsize;
      if (mv[3] == 0xCE) {
        minsize = sizeof(mach_header);
      } else {
        minsize = sizeof(mach_header_64);
      }
      if (mv.size() >= minsize)
        type = mv[12] << 24 | mv[13] << 12 | mv[14] << 8 | mv[15];
    } else if (mv.StartsWith("\xCE\xFA\xED\xFE") || mv.StartsWith("\xCF\xFA\xED\xFE")) {
      /* Reverse endian */
      size_t minsize;
      if (mv[0] == 0xCE) {
        minsize = sizeof(mach_header);
      } else {
        minsize = sizeof(mach_header_64);
      }
      if (mv.size() >= minsize) {
        type = mv[15] << 24 | mv[14] << 12 | mv[13] << 8 | mv[12];
      }
    }
    switch (type) {
    default:
      break;
    case 1:
      hr.assign(types::macho_object, L"Mach-O Object file");
      return Found;
    case 2:
      hr.assign(types::macho_executable, L"Mach-O Executable");
      return Found;
    case 3:
      hr.assign(types::macho_fixed_virtual_memory_shared_lib, L"Mach-O Shared Lib, FVM");
      return Found;
    case 4:
      hr.assign(types::macho_core, L"Mach-O Core File");
      return Found;
    case 5:
      hr.assign(types::macho_preload_executable, L"Mach-O Preloaded Executable");
      return Found;
    case 6:
      hr.assign(types::macho_dynamically_linked_shared_lib, L"Mach-O dynlinked shared lib");
      return Found;
    case 7:
      hr.assign(types::macho_dynamic_linker, L"The Mach-O dynamic linker");
      return Found;
    case 8:
      hr.assign(types::macho_bundle, L"Mach-O Bundle file");
      return Found;
    case 9:
      hr.assign(types::macho_dynamically_linked_shared_lib_stub, L"Mach-O Shared lib stub");
      return Found;
    case 10:
      hr.assign(types::macho_dsym_companion, L"Mach-O dSYM companion file");
      return Found;
    case 11:
      hr.assign(types::macho_kext_bundle, L"Mach-O kext bundle file");
      return Found;
    }
    break;
  }
  case 0xF0: // PowerPC Windows
  case 0x83: // Alpha 32-bit
  case 0x84: // Alpha 64-bit
  case 0x66: // MPS R4000 Windows
  case 0x50: // mc68K
  case 0x4c: // 80386 Windows
  case 0xc4: // ARMNT Windows
    if (mv[1] == 0x01) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    [[fallthrough]];
  case 0x90: // PA-RISC Windows
  case 0x68: // mc68K Windows
    if (mv[1] == 0x02) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    break;
  case 'M':
    if (mv.StartsWith("Microsoft C/C++ MSF 7.00\r\n")) {
      hr.assign(types::pdb, L"Windows PDB debug info file");
      return Found;
    }
    if (mv.StartsWith("MZ") && mv.size() >= 0x3c + 4) {
      // read32le
      uint32_t off = bela::cast_fromle<uint32_t>(mv.data() + 0x3c);
      auto sv = mv.submv(off);
      if (sv.StartsWith(PEMagic)) {
        hr.assign(types::pecoff_executable, L"PE executable file");
        return Found;
      }
    }
    break;
  case 0x64: // x86-64 or ARM64 Windows.
    if (mv[1] == 0x86 || mv[1] == 0xaa) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    break;
  default:
    break;
  }
  return None;
}
} // namespace hazel::internal
