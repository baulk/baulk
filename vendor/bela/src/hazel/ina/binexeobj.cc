//
#include "hazelinc.hpp"

namespace hazel::internal {
static constexpr const uint8_t ElfMagic[] = {0x7f, 'E', 'L', 'F'};

constexpr auto FF = (std::numeric_limits<uint8_t>::max)();

// The PE signature bytes that follows the DOS stub header.
static constexpr const uint8_t PEMagic[] = {'P', 'E', '\0', '\0'};
static constexpr const uint8_t BigObjMagic[] = {
    0xc7, 0xa1, 0xba, 0xd1, 0xee, 0xba, 0xa9, 0x4b, 0xaf, 0x20, 0xfa, 0xf6, 0x6a, 0xa4, 0xdc, 0xb8,
};
static constexpr const uint8_t ClGlObjMagic[] = {
    0x38, 0xfe, 0xb3, 0x0c, 0xa5, 0xd9, 0xab, 0x4d, 0xac, 0x9b, 0xd6, 0xb6, 0x22, 0x26, 0x53, 0xc2,
};
// The signature bytes that start a .res file.
static constexpr const uint8_t WinResMagic[] = {
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, FF, FF, 0x00, 0x00, FF, FF, 0x00, 0x00,
};
static constexpr const uint8_t debMagic[] = {0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A, 0x64, 0x65, 0x62,
                                             0x69, 0x61, 0x6E, 0x2D, 0x62, 0x69, 0x6E, 0x61, 0x72, 0x79};
static constexpr const uint8_t ifcMagic[] = {0x54, 0x51, 0x45, 0x1a};
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

constexpr std::u8string_view wasmobj{u8"\0asm", 4};
constexpr std::u8string_view irobj{u8"\xDE\xC0\x17\x0B", 4};
constexpr std::u8string_view irobj2{u8"\xBC\xC0\xDE", 3};
constexpr std::u8string_view bobj{u8"\0\0\xFF\xFF", 4};
constexpr std::u8string_view xo32{u8"\x01\xDF", 2};
constexpr std::u8string_view xo64{u8"\x01\xF7", 2};
constexpr uint8_t goffmagic[] = {0x03, 0xF0, 0x00};

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

inline status_t macho_resolve(uint16_t type, hazel_result &hr) {
  switch (type) {
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
  default:
    break;
  }
  return None;
}

status_t LookupExecutableFile(const bela::bytes_view &bv, hazel_result &hr) {
  if (bv.size() < 4) {
    return None;
  }
  switch (bv[0]) {
  case 0x00:
    if (bv.starts_with(bobj)) {
      size_t minsize = offsetof(BigObjHeader, UUID) + sizeof(BigObjMagic);
      if (bv.size() < minsize) {
        hr.assign(types::coff_import_library, L"COFF import library");
        return Found;
      }
      const char *start = reinterpret_cast<const char *>(bv.data()) + offsetof(BigObjHeader, UUID);
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
    if (bv.size() >= sizeof(WinResMagic) && memcmp(bv.data(), WinResMagic, sizeof(WinResMagic)) == 0) {
      hr.assign(types::windows_resource, L"Windows compiled resource file (.res)");
      return Found;
    }
    // if (mv[1] == 0) {
    //   hr.assign(L"COFF object", types::coff_object);
    //   return Found;
    // }
    if (bv.starts_with(wasmobj)) {
      hr.assign(types::wasm_object, L"WebAssembly Object file");
      return Found;
    }
    break;
  case 0x01:
    if (bv.starts_with(xo32)) {
      hr.assign(types::xcoff_object_32, L"32-bit XCOFF object file");
      return Found;
    }
    if (bv.starts_with(xo64)) {
      hr.assign(types::xcoff_object_64, L"64-bit XCOFF object file");
      return Found;
    }
    break;
  case 0x03:
    if (bv.starts_bytes_with(goffmagic)) {
      hr.assign(types::goff_object, L"[SystemZ][z/OS] GOFF object");
      return Found;
    }
    break;
  case 0xDE:
    if (bv.starts_with(irobj)) {
      hr.assign(types::bitcode, L"LLVM IR bitcode");
      return Found;
    }
    break;
  case 'B':
    if (bv.starts_with(irobj2)) {
      hr.assign(types::bitcode, L"LLVM IR bitcode");
      return Found;
    }
    break;
  case '!': // .a
    if ((bv.starts_with("!<arch>\n") && !bv.starts_bytes_with(debMagic)) || bv.starts_with("!<thin>\n")) {
      // Skip DEB package
      hr.assign(types::archive, L"ar style archive file");
      return Found;
    }
    break;
  case '\177': // ELF
    if (bv.starts_bytes_with(ElfMagic) && bv.size() >= 18) {
      bool Data2MSB = (bv[5] == 2);
      unsigned high = Data2MSB ? 16 : 17;
      unsigned low = Data2MSB ? 17 : 16;
      if (bv[high] == 0) {
        switch (bv[low]) {
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
      hr.assign(types::elf, L"ELF Unknown type");
      return Found;
    }
    break;
  case 0xCA:
    if (bv.starts_with("\xCA\xFE\xBA\xBE") || bv.starts_with("\xCA\xFE\xBA\xBF")) {
      if (bv.size() >= 8 && bv[7] < 43) {
        hr.assign(types::macho_universal_binary, L"Mach-O universal binary");
        return Found;
      }
    }
    break;
  case 0xFE:
    [[fallthrough]];
  case 0xCE:
    [[fallthrough]];
  case 0xCF: {
    uint16_t type = 0;
    if (bv.starts_with("\xFE\xED\xFA\xCE") || bv.starts_with("\xFE\xED\xFA\xCF")) {
      /* Native endian */
      size_t minsize;
      if (bv[3] == 0xCE) {
        minsize = sizeof(mach_header);
      } else {
        minsize = sizeof(mach_header_64);
      }
      if (bv.size() >= minsize) {
        type = bv[12] << 24 | bv[13] << 12 | bv[14] << 8 | bv[15];
      }
      return macho_resolve(type, hr);
    }
    if (bv.starts_with("\xCE\xFA\xED\xFE") || bv.starts_with("\xCF\xFA\xED\xFE")) {
      /* Reverse endian */
      size_t minsize;
      if (bv[0] == 0xCE) {
        minsize = sizeof(mach_header);
      } else {
        minsize = sizeof(mach_header_64);
      }
      if (bv.size() >= minsize) {
        type = bv[15] << 24 | bv[14] << 12 | bv[13] << 8 | bv[12];
      }
      return macho_resolve(type, hr);
    }
  } break;
  case 0xF0: // PowerPC Windows
    [[fallthrough]];
  case 0x83: // Alpha 32-bit
    [[fallthrough]];
  case 0x84: // Alpha 64-bit
    [[fallthrough]];
  case 0x66: // MPS R4000 Windows
    [[fallthrough]];
  case 0x50: // mc68K
    [[fallthrough]];
  case 0x4c: // 80386 Windows
    [[fallthrough]];
  case 0xc4: // ARMNT Windows
    if (bv[1] == 0x01) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    [[fallthrough]];
  case 0x90: // PA-RISC Windows
    [[fallthrough]];
  case 0x68: // mc68K Windows
    if (bv[1] == 0x02) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    break;
  case 'M':
    if (bv.starts_with("MZ") && bv.size() >= 0x3c + 4) {
      // read32le
      auto off = bela::cast_fromle<uint32_t>(bv.data() + 0x3c);
      auto sv = bv.subview(off);
      if (sv.starts_bytes_with(PEMagic)) {
        hr.assign(types::pecoff_executable, L"PE executable file");
        return Found;
      }
    }
    if (bv.starts_with("Microsoft C/C++ MSF 7.00\r\n")) {
      hr.assign(types::pdb, L"Windows PDB debug info file");
      return Found;
    }
    if (bv.starts_with("MDMP")) {
      hr.assign(types::minidump, L"Windows minidump file");
      return Found;
    }
    break;
  case 0x64: // x86-64 or ARM64 Windows.
    if (bv[1] == 0x86 || bv[1] == 0xaa) {
      hr.assign(types::coff_object, L"COFF object");
      return Found;
    }
    break;
  case 0x54:
    if (bv.starts_bytes_with(ifcMagic)) {
      hr.assign(types::ifc, L"MSVC IFC (C++ module binary)");
      return Found;
    }
    break;
  default:
    break;
  }
  return None;
}
} // namespace hazel::internal
