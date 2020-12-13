///
#ifndef BELA_PE_HPP
#define BELA_PE_HPP
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include "base.hpp"
#include "endian.hpp"
#include "phmap.hpp"
#include "types.hpp"
#include "ascii.hpp"
#include "match.hpp"

namespace bela::pe {
enum class Machine : uint16_t {
  UNKNOWN = 0,
  TARGET_HOST = 0x0001, // Useful for indicating we want to interact with the
                        // host and not a WoW guest.
  I386 = 0x014c,        // Intel 386.
  R3000 = 0x0162,       // MIPS little-endian, 0x160 big-endian
  R4000 = 0x0166,       // MIPS little-endian
  R10000 = 0x0168,      // MIPS little-endian
  WCEMIPSV2 = 0x0169,   // MIPS little-endian WCE v2
  ALPHA = 0x0184,       // Alpha_AXP
  SH3 = 0x01a2,         // SH3 little-endian
  SH3DSP = 0x01a3,
  SH3E = 0x01a4,  // SH3E little-endian
  SH4 = 0x01a6,   // SH4 little-endian
  SH5 = 0x01a8,   // SH5
  ARM = 0x01c0,   // ARM Little-Endian
  THUMB = 0x01c2, // ARM Thumb/Thumb-2 Little-Endian
  ARMNT = 0x01c4, // ARM Thumb-2 Little-Endian
  AM33 = 0x01d3,
  POWERPC = 0x01F0, // IBM PowerPC Little-Endian
  POWERPCFP = 0x01f1,
  IA64 = 0x0200,      // Intel 64
  MIPS16 = 0x0266,    // MIPS
  ALPHA64 = 0x0284,   // ALPHA64
  MIPSFPU = 0x0366,   // MIPS
  MIPSFPU16 = 0x0466, // MIPS
  TRICORE = 0x0520,   // Infineon
  CEF = 0x0CEF,
  EBC = 0x0EBC,   // EFI Byte Code
  AMD64 = 0x8664, // AMD64 (K8)
  M32R = 0x9041,  // M32R little-endian
  ARM64 = 0xAA64, // ARM64 Little-Endian
  RISCV32 = 0x5032,
  RISCV64 = 0x5064,
  RISCV128 = 0x5128,
  CHPEX86 = 0x3A64,
  CEE = 0xC0EE
};
enum class Subsystem : uint16_t {
  UNKNOWN = 0,
  NATIVE = 1,
  GUI = 2,
  CUI = 3, //
  OS2_CUI = 5,
  POSIX_CUI = 7,
  NATIVE_WINDOWS = 8,
  WINDOWS_CE_GUI = 9,
  EFI_APPLICATION = 10,
  EFI_BOOT_SERVICE_DRIVER = 11,
  EFI_RUNTIME_DRIVER = 12,
  EFI_ROM = 13,
  XBOX = 14,
  WINDOWS_BOOT_APPLICATION = 16,
  XBOX_CODE_CATALOG = 17
};
#pragma pack(1)

struct DosHeader {     // DOS .EXE header
  uint16_t e_magic;    // Magic number
  uint16_t e_cblp;     // Bytes on last page of file
  uint16_t e_cp;       // Pages in file
  uint16_t e_crlc;     // Relocations
  uint16_t e_cparhdr;  // Size of header in paragraphs
  uint16_t e_minalloc; // Minimum extra paragraphs needed
  uint16_t e_maxalloc; // Maximum extra paragraphs needed
  uint16_t e_ss;       // Initial (relative) SS value
  uint16_t e_sp;       // Initial SP value
  uint16_t e_csum;     // Checksum
  uint16_t e_ip;       // Initial IP value
  uint16_t e_cs;       // Initial (relative) CS value
  uint16_t e_lfarlc;   // File address of relocation table
  uint16_t e_ovno;     // Overlay number
  uint16_t e_res[4];   // Reserved words
  uint16_t e_oemid;    // OEM identifier (for e_oeminfo)
  uint16_t e_oeminfo;  // OEM information; e_oemid specific
  uint16_t e_res2[10]; // Reserved words
  uint32_t e_lfanew;   // File address of new exe header
};

struct FileHeader {
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
};
struct DataDirectory {
  uint32_t VirtualAddress;
  uint32_t Size;
};

struct OptionalHeader32 {
  uint16_t Magic;
  uint8_t MajorLinkerVersion;
  uint8_t MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint32_t BaseOfData;
  uint32_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint32_t SizeOfStackReserve;
  uint32_t SizeOfStackCommit;
  uint32_t SizeOfHeapReserve;
  uint32_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectory[16];
};

struct OptionalHeader64 {
  uint16_t Magic;
  uint8_t MajorLinkerVersion;
  uint8_t MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint64_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint64_t SizeOfStackReserve;
  uint64_t SizeOfStackCommit;
  uint64_t SizeOfHeapReserve;
  uint64_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectory[16];
};

struct SectionHeader32 {
  uint8_t Name[8];
  uint32_t VirtualSize;
  uint32_t VirtualAddress;
  uint32_t SizeOfRawData;
  uint32_t PointerToRawData;
  uint32_t PointerToRelocations;
  uint32_t PointerToLineNumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLineNumbers;
  uint32_t Characteristics;
};

struct Reloc {
  uint32_t VirtualAddress;
  uint32_t SymbolTableIndex;
  uint16_t Type;
};

// COFFSymbol represents single COFF symbol table record.
struct COFFSymbol {
  uint8_t Name[8];
  uint32_t Value;
  int16_t SectionNumber;
  uint16_t Type;
  uint8_t StorageClass;
  uint8_t NumberOfAuxSymbols;
};

#define STORAGE_MAGIC_SIG 0x424A5342 // BSJB
struct STORAGESIGNATURE {
  uint32_t lSignature;     // "Magic" signature.
  uint16_t iMajorVer;      // Major file version.
  uint16_t iMinorVer;      // Minor file version.
  uint32_t iExtraData;     // Offset to next structure of information
  uint32_t iVersionString; // Length of version string
};

#pragma pack()

struct SectionHeader {
  std::string Name;
  uint32_t VirtualSize;
  uint32_t VirtualAddress;
  uint32_t Size;
  uint32_t Offset;
  uint32_t PointerToRelocations;
  uint32_t PointerToLineNumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLineNumbers;
  uint32_t Characteristics;
};

struct Section {
  SectionHeader Header;
  std::vector<Reloc> Relocs;
};

constexpr uint32_t COFFSymbolSize = sizeof(COFFSymbol);

// StringTable: Programs written in golang will customize stringtable
struct StringTable {
  uint8_t *data{nullptr};
  size_t length{0};
  StringTable() = default;
  StringTable(const StringTable &) = delete;
  StringTable &operator=(const StringTable &) = delete;
  StringTable(StringTable &&other) { MoveFrom(std::move(other)); }
  StringTable &operator=(StringTable &&other) {
    MoveFrom(std::move(other));
    return *this;
  }
  ~StringTable();
  void MoveFrom(StringTable &&other);
  std::string String(uint32_t start, bela::error_code &ec) const;
};

// Symbol is similar to COFFSymbol with Name field replaced
// by Go string. Symbol also does not have NumberOfAuxSymbols.
struct Symbol {
  std::string Name;
  uint32_t Value;
  int16_t SectionNumber;
  uint16_t Type;
  uint8_t StorageClass;
};

struct ExportedSymbol {
  std::string Name;
  std::string UndecoratedName;
  std::string ForwardName;
  DWORD Address;
  unsigned short Ordinal{0xFFFF};
  int Hint{0};
};

struct Function {
  Function(std::string &&name, int index = 0, int ordinal = 0)
      : Name(std::move(name)), Index(index), Ordinal(ordinal) {}
  Function(const std::string_view &name, int index = 0, int ordinal = 0) : Name(name), Index(index), Ordinal(ordinal) {}
  std::string Name;
  int Index{0};
  int Ordinal{0};
};

struct FunctionTable {
  using symbols_map_t = bela::flat_hash_map<std::string, std::vector<Function>>;
  symbols_map_t imports;
  symbols_map_t delayimprots;
  std::vector<ExportedSymbol> exports;
};

using symbols_map_t = bela::flat_hash_map<std::string, std::vector<Function>>;

class File {
private:
  bool ParseFile(bela::error_code &ec);
  bool PositionAt(uint64_t pos, bela::error_code &ec) const {
    LARGE_INTEGER oli{0};
    if (SetFilePointerEx(fd, *reinterpret_cast<LARGE_INTEGER *>(&pos), &oli, SEEK_SET) != TRUE) {
      ec = bela::make_system_error_code(L"SetFilePointerEx: ");
      return false;
    }
    return true;
  }
  bool Read(void *buffer, size_t len, size_t &outlen, bela::error_code &ec) const {
    DWORD dwSize = {0};
    if (ReadFile(fd, buffer, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    outlen = static_cast<size_t>(len);
    return true;
  }
  bool ReadFull(void *buffer, size_t len, bela::error_code &ec) const {
    auto p = reinterpret_cast<uint8_t *>(buffer);
    size_t total = 0;
    while (total < len) {
      DWORD dwSize = 0;
      if (ReadFile(fd, p + total, static_cast<DWORD>(len - total), &dwSize, nullptr) != TRUE) {
        ec = bela::make_system_error_code(L"ReadFile: ");
        return false;
      }
      if (dwSize == 0) {
        ec = bela::make_error_code(ERROR_HANDLE_EOF, L"Reached the end of the file");
        return false;
      }
      total += dwSize;
    }
    return true;
  }
  // ReadAt ReadFull
  bool ReadAt(void *buffer, size_t len, uint64_t pos, bela::error_code &ec) const {
    if (!PositionAt(pos, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  std::string sectionFullName(SectionHeader32 &sh) const;
  bool readCOFFSymbols(std::vector<COFFSymbol> &symbols, bela::error_code &ec) const;
  bool readRelocs(Section &sec) const;
  bool readSectionData(const Section &sec, std::vector<char> &data) const;
  bool readStringTable(bela::error_code &ec);
  bool LookupDelayImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;
  bool LookupImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;

public:
  File() = default;
  ~File() {
    if (fd != INVALID_HANDLE_VALUE && needClosed) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  File(const File &) = delete;
  File &operator=(const File &&) = delete;
  // export FD() to support
  HANDLE FD() const { return fd; }
  template <typename AStringT> void SplitStringTable(std::vector<AStringT> &sa) const {
    auto sv = std::string_view{reinterpret_cast<const char *>(stringTable.data), stringTable.length};
    for (;;) {
      auto p = sv.find('\0');
      if (p == std::string_view::npos) {
        if (sv.size() != 0) {
          sa.emplace_back(sv);
        }
        break;
      }
      sa.emplace_back(sv.substr(0, p));
      sv.remove_prefix(p + 1);
    }
  }
  bool LookupExports(std::vector<ExportedSymbol> &exports, bela::error_code &ec) const;
  bool LookupFunctionTable(FunctionTable &ft, bela::error_code &ec) const;
  bool LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const;
  bool LookupClrVersion(std::string &ver, bela::error_code &ec) const;
  const FileHeader &Fh() const { return fh; }
  const OptionalHeader64 *Oh64() const { return &oh; }
  const OptionalHeader32 *Oh32() const { return reinterpret_cast<const OptionalHeader32 *>(&oh); }
  const auto &Sections() const { return sections; }
  bool Is64Bit() const { return is64bit; }
  bela::pe::Machine Machine() const { return static_cast<bela::pe::Machine>(fh.Machine); }
  bela::pe::Subsystem Subsystem() const {
    return static_cast<bela::pe::Subsystem>(is64bit ? oh.Subsystem : Oh32()->Subsystem);
  }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec) {
    if (fd != INVALID_HANDLE_VALUE) {
      ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
      return false;
    }
    fd = fd_;
    size = sz;
    return ParseFile(ec);
  }
  int64_t Size() const { return size; }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  FileHeader fh;
  int64_t size{SizeUnInitialized};
  // The OptionalHeader64 structure is larger than OptionalHeader32. Therefore, we can store OptionalHeader32 in oh64.
  // Conversion by pointer.
  OptionalHeader64 oh;
  std::vector<Section> sections;
  StringTable stringTable;
  bool is64bit{false};
  bool needClosed{false};
};

class SymbolSearcher {
private:
  using SymbolTable = bela::flat_hash_map<std::string, std::vector<bela::pe::ExportedSymbol>>;
  SymbolTable table;
  std::vector<std::wstring> Paths;
  std::optional<std::string> LoadOrdinalFunctionName(std::string_view dllname, int ordinal, bela::error_code &ec);

public:
  SymbolSearcher(std::wstring_view exe, Machine machine);
  SymbolSearcher(std::vector<std::wstring> &&paths) : Paths(std::move(paths)) {}
  SymbolSearcher(const SymbolSearcher &) = delete;
  SymbolSearcher &operator=(const SymbolSearcher &) = delete;
  std::optional<std::string> LookupOrdinalFunctionName(std::string_view dllname, int ordinal, bela::error_code &ec);
};

struct FileInfo {
  DWORD dwSignature;        /* e.g. 0xfeef04bd */
  DWORD dwStrucVersion;     /* e.g. 0x00000042 = "0.42" */
  DWORD dwFileVersionMS;    /* e.g. 0x00030075 = "3.75" */
  DWORD dwFileVersionLS;    /* e.g. 0x00000031 = "0.31" */
  DWORD dwProductVersionMS; /* e.g. 0x00030010 = "3.10" */
  DWORD dwProductVersionLS; /* e.g. 0x00000031 = "0.31" */
  DWORD dwFileFlagsMask;    /* = 0x3F for version "0.42" */
  DWORD dwFileFlags;        /* e.g. VFF_DEBUG | VFF_PRERELEASE */
  DWORD dwFileOS;           /* e.g. VOS_DOS_WINDOWS16 */
  DWORD dwFileType;         /* e.g. VFT_DRIVER */
  DWORD dwFileSubtype;      /* e.g. VFT2_DRV_KEYBOARD */
  DWORD dwFileDateMS;       /* e.g. 0 */
  DWORD dwFileDateLS;       /* e.g. 0 */
};

// https://docs.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfoexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-getfileversioninfosizeexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-verqueryvaluew
// version.lib
struct Version {
  std::wstring CompanyName;
  std::wstring FileDescription;
  std::wstring FileVersion;
  std::wstring InternalName;
  std::wstring LegalCopyright;
  std::wstring OriginalFileName;
  std::wstring ProductName;
  std::wstring ProductVersion;
  std::wstring Comments;
  std::wstring LegalTrademarks;
  std::wstring PrivateBuild;
  std::wstring SpecialBuild;
};

std::optional<Version> Lookup(std::wstring_view file, bela::error_code &ec);

inline bool IsSubsystemConsole(std::wstring_view p) {
  constexpr const wchar_t *suffix[] = {
      // console suffix
      L".bat", // batch
      L".cmd", // batch
      L".vbs", // Visual Basic script files
      L".vbe", // Visual Basic script files (encrypted)
      L".js",  // JavaScript
      L".jse", // JavaScript (encrypted)
      L".wsf", // WScript
      L".wsh", // Windows Script Host Settings File
  };
  File file;
  bela::error_code ec;
  if (!file.NewFile(p, ec)) {
    auto lp = bela::AsciiStrToLower(p);
    for (const auto s : suffix) {
      if (bela::EndsWith(lp, s)) {
        return true;
      }
    }
    return false;
  }
  return file.Subsystem() == Subsystem::CUI;
}

} // namespace bela::pe

#endif