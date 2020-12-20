//
#ifndef HAZEL_MACHO_HPP
#define HAZEL_MACHO_HPP
#include <bela/endian.hpp>
#include "hazel.hpp"
#include "details/macho.h"

namespace hazel::macho {

constexpr size_t fileHeaderSize32 = 7 * 4;
constexpr size_t fileHeaderSize64 = 8 * 4;

constexpr uint32_t Magic32 = 0xfeedface;
constexpr uint32_t Magic64 = 0xfeedfacf;
constexpr uint32_t MagicFat = 0xcafebabe;

constexpr uint32_t MachoObject = 1;
constexpr uint32_t MachoExec = 2;
constexpr uint32_t MachoDylib = 6;
constexpr uint32_t MachoBundle = 8;
constexpr uint32_t Machine64 = 0x01000000;

enum LoadCmd : uint32_t {
  LoadCmdSegment = 0x1,
  LoadCmdSymtab = 0x2,
  LoadCmdThread = 0x4,
  LoadCmdUnixThread = 0x5, // thread+stack
  LoadCmdDysymtab = 0xb,
  LoadCmdDylib = 0xc,    // load dylib command
  LoadCmdDylinker = 0xf, // id dylinker command (not load dylinker command)
  LoadCmdSegment64 = 0x19,
  LoadCmdRpath = 0x8000001c,
};

#pragma pack(4)
struct FileHeader {
  uint32_t Magic;
  uint32_t Cpu;
  uint32_t SubCpu;
  uint32_t Type;
  uint32_t Ncmd;
  uint32_t Cmdsz;
  uint32_t Flags;
};

// A Segment32 is a 32-bit Mach-O segment load command.
struct Segment32 {
  LoadCmd Cmd;
  uint32_t Len;
  uint8_t Name[16];
  uint32_t Addr;
  uint32_t Memsz;
  uint32_t Offset;
  uint32_t Filesz;
  uint32_t Maxprot;
  uint32_t Prot;
  uint32_t Nsect;
  uint32_t Flag;
};

// A Segment64 is a 64-bit Mach-O segment load command.
struct Segment64 {
  LoadCmd Cmd;
  uint32_t Len;
  uint8_t Name[16];
  uint64_t Addr;
  uint64_t Memsz;
  uint64_t Offset;
  uint64_t Filesz;
  uint32_t Maxprot;
  uint32_t Prot;
  uint32_t Nsect;
  uint32_t Flag;
};

// A SymtabCmd is a Mach-O symbol table command.
struct SymtabCmd {
  LoadCmd Cmd;
  uint32_t Len;
  uint32_t Symoff;
  uint32_t Nsyms;
  uint32_t Stroff;
  uint32_t Strsize;
};

// A DysymtabCmd is a Mach-O dynamic symbol table command.
struct DysymtabCmd {
  LoadCmd Cmd;
  uint32_t Len;
  uint32_t Ilocalsym;
  uint32_t Nlocalsym;
  uint32_t Iextdefsym;
  uint32_t Nextdefsym;
  uint32_t Iundefsym;
  uint32_t Nundefsym;
  uint32_t Tocoffset;
  uint32_t Ntoc;
  uint32_t Modtaboff;
  uint32_t Nmodtab;
  uint32_t Extrefsymoff;
  uint32_t Nextrefsyms;
  uint32_t Indirectsymoff;
  uint32_t Nindirectsyms;
  uint32_t Extreloff;
  uint32_t Nextrel;
  uint32_t Locreloff;
  uint32_t Nlocrel;
};

struct RpathCmd {
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Path;
};
struct DylibCmd {
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Name;
  uint32_t Time;
  uint32_t CurrentVersion;
  uint32_t CompatVersion;
};

struct Thread {
  LoadCmd Cmd;
  uint32_t Len;
  uint32_t Type;
  uint32_t Data[1]; // Variable length array
};

// A Section32 is a 32-bit Mach-O section header.
struct Section32 {
  uint8_t Name[16];
  uint8_t Seg[16];
  uint32_t Addr;
  uint32_t Size;
  uint32_t Offset;
  uint32_t Align;
  uint32_t Reloff;
  uint32_t Nreloc;
  uint32_t Flags;
  uint32_t Reserve1;
  uint32_t Reserve2;
};

// A Section64 is a 64-bit Mach-O section header.
struct Section64 {
  uint8_t Name[16];
  uint8_t Seg[16];
  uint64_t Addr;
  uint64_t Size;
  uint32_t Offset;
  uint32_t Align;
  uint32_t Reloff;
  uint32_t Nreloc;
  uint32_t Flags;
  uint32_t Reserve1;
  uint32_t Reserve2;
  uint32_t Reserve3;
};

// An Nlist32 is a Mach-O 32-bit symbol table entry.
struct Nlist32 {
  uint32_t Name;
  uint8_t Type;
  uint8_t Sect;
  uint16_t Desc;
  uint32_t Value;
};

// An Nlist64 is a Mach-O 64-bit symbol table entry.
struct Nlist64 {
  uint32_t Name;
  uint8_t Type;
  uint8_t Sect;
  uint16_t Desc;
  uint64_t Value;
};

#pragma pack()

enum Machine : uint32_t {
  VAX = 1,
  MC680x0 = 0,
  I386 = 7,
  MC98000 = 9,
  HPPA = 11,
  MC88000 = 13,
  ARM = 12,
  SPARC = 14,
  I860 = 15,
  POWERPC = 18,
  AMD64 = I386 | Machine64,
  ARM64 = ARM | Machine64,
  POWERPC64 = POWERPC | Machine64,
};

struct Segment {
  std::string Bytes;
  std::string Name;
  uint32_t Cmd;
  uint32_t Len;
  uint64_t Addr;
  uint64_t Memsz;
  uint64_t Offset;
  uint64_t Filesz;
  uint32_t Maxprot;
  uint32_t Prot;
  uint32_t Nsect;
  uint32_t Flag;
};

struct Reloc {
  uint32_t Addr;
  uint32_t Value;
  uint8_t Type;
  uint8_t Len; // 0=byte, 1=word, 2=long, 3=quad
  bool Pcrel;
  bool Extern; // valid if Scattered == false
  bool Scattered;
};

struct Section {
  std::string Name;
  std::string Seg;
  uint64_t Addr;
  uint64_t Size;
  uint32_t Offset;
  uint32_t Align;
  uint32_t Reloff;
  uint32_t Nreloc;
  uint32_t Flags;
  std::vector<Reloc> Relocs;
};

struct Dylib {
  std::string Name;
  uint32_t NameIndex;
  uint32_t CurrentVersion;
  uint32_t CompatVersion;
};

struct Symbol {
  std::string Name;
  uint8_t Type;
  uint8_t Sect;
  uint16_t Desc;
  uint64_t Value;
};

struct Symtab {
  std::string Bytes;
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Symoff;
  uint32_t Nsyms;
  uint32_t Stroff;
  uint32_t Strsize;
  std::vector<Symbol> Syms;
};

struct Dysymtab {
  std::string Bytes;
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Ilocalsym;
  uint32_t Nlocalsym;
  uint32_t Iextdefsym;
  uint32_t Nextdefsym;
  uint32_t Iundefsym;
  uint32_t Nundefsym;
  uint32_t Tocoffset;
  uint32_t Ntoc;
  uint32_t Modtaboff;
  uint32_t Nmodtab;
  uint32_t Extrefsymoff;
  uint32_t Nextrefsyms;
  uint32_t Indirectsymoff;
  uint32_t Nindirectsyms;
  uint32_t Extreloff;
  uint32_t Nextrel;
  uint32_t Locreloff;
  uint32_t Nlocrel;
  std::vector<uint32_t> IndirectSyms;
};

struct DyLib {
  std::string Name;
  uint32_t Time;
  uint32_t CurrentVersion;
  uint32_t CompatVersion;
};

struct Load {
  void Free() {
    switch (Cmd) {
    case LoadCmdRpath:
      delete Path;
      break;
    case LoadCmdDylib:
      delete DyLib;
      break;
    case LoadCmdSegment64:
      [[fallthrough]];
    case LoadCmdSegment:
      delete Segment;
      break;
    default:
      break;
    }
  }
  Load() { Path = nullptr; }
  Load(const Load &r) = delete;
  Load &operator=(const Load &r) = delete;
  Load(Load &&r) { MoveFrom(std::move(r)); }
  Load &operator=(Load &&r) {
    MoveFrom(std::move(r));
    return *this;
  }
  void MoveFrom(Load &&r) {
    bytes = std::move(r.bytes);
    switch (Cmd) {
    case LoadCmdRpath:
      delete Path;
      Path = r.Path;
      r.Path = nullptr;
      break;
    case LoadCmdDylib:
      delete DyLib;
      DyLib = r.DyLib;
      r.DyLib = nullptr;
      break;
    case LoadCmdSegment64:
      [[fallthrough]];
    case LoadCmdSegment:
      delete Segment;
      Segment = r.Segment;
      r.Segment = nullptr;
      break;
    default:
      break;
    }
  }
  ~Load() { Free(); }
  std::string bytes;
  uint32_t Cmd{0};
  union {
    std::string *Path;
    hazel::macho::DyLib *DyLib;
    hazel::macho::Segment *Segment;
  };
};

class FatFile;

constexpr auto ErrNotFat = static_cast<long>(MagicFat);

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
  bool ReadAt(void *buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos + baseOffset, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  bool ReadAt(bela::Buffer &buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos + baseOffset, ec)) {
      return false;
    }
    if (!ReadFull(buffer.data(), len, ec)) {
      return false;
    }
    buffer.size() = len;
    return true;
  }

  void Free() {
    if (needClosed && fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  void MoveFrom(File &&r) {
    Free();
    fd = r.fd;
    r.fd = INVALID_HANDLE_VALUE;
    needClosed = r.needClosed;
    r.needClosed = false;
    size = r.size;
    r.size = 0;
    baseOffset = r.baseOffset;
    r.baseOffset = 0;
    loads = std::move(r.loads);
    dysymtab = std::move(r.dysymtab);
    symtab = std::move(r.symtab);
    sections = std::move(r.sections);
    memcpy(&fh, &r.fh, sizeof(fh));
    memset(&r.fh, 0, sizeof(r.fh));
  }

  template <typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
  Integer endian_cast(Integer t) {
    if (en == std::endian::native) {
      return t;
    }
    return bela::bswap(t);
  }
  template <typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
  Integer cast_from(const void *p) {
    auto v = bela::unaligned_load<Integer>(p);
    if (en == std::endian::native) {
      return v;
    }
    return bela::bswap(v);
  }
  bool readFileHeader(int64_t &offset, bela::error_code &ec);
  bool parseSymtab(std::string_view symdat, std::string_view strtab, std::string_view cmddat, const SymtabCmd &hdr,
                   int64_t offset, bela::error_code &ec);
  bool pushSection(hazel::macho::Section *sh, bela::error_code &ec);

public:
  File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  File(File &&r) { MoveFrom(std::move(r)); }
  File &operator=(File &&r) {
    MoveFrom(std::move(r));
    return *this;
  }
  ~File() { Free(); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  bool Is64Bit() const { return is64bit; }
  int64_t Size() const { return size; }
  bool Depends(std::vector<std::string> &libs, bela::error_code &ec);
  bool ImportedSymbols(std::vector<std::string> &symbols, bela::error_code &ec);
  const hazel::macho::Section *Section(std::string_view name) const {
    for (const auto &s : sections) {
      if (s.Name == name) {
        return &s;
      }
    }
    return nullptr;
  }
  const hazel::macho::Segment *Segment(std::string_view name) const {
    for (const auto &l : loads) {
      if ((l.Cmd == LoadCmdSegment || l.Cmd == LoadCmdSegment64) && l.Segment != nullptr) {
        if (l.Segment->Name == name) {
          return l.Segment;
        }
      }
    }
    return nullptr;
  }

private:
  friend class FatFile;
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t baseOffset{0}; // when support fat
  int64_t size{bela::SizeUnInitialized};
  std::endian en{std::endian::native};
  std::vector<Load> loads;
  std::vector<hazel::macho::Section> sections;
  FileHeader fh;
  Symtab symtab;
  Dysymtab dysymtab;
  bool is64bit{false};
  bool needClosed{false};
};

struct FatArchHeader {
  uint32_t Cpu;
  uint32_t SubCpu;
  uint32_t Offset;
  uint32_t Size;
  uint32_t Align;
};

struct FatArch {
  FatArchHeader fh;
  File file;
};

class FatFile {
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
  bool ReadAt(void *buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  bool ReadAt(bela::Buffer &buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos, ec)) {
      return false;
    }
    if (!ReadFull(buffer.data(), len, ec)) {
      return false;
    }
    buffer.size() = len;
    return true;
  }

  void Free() {
    if (needClosed && fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  void MoveFrom(File &&r) {
    Free();
    fd = r.fd;
    r.fd = INVALID_HANDLE_VALUE;
    r.needClosed = false;
    size = r.size;
    arches = std::move(arches);
  }

public:
  FatFile() = default;
  FatFile(const FatFile &) = delete;
  FatFile &operator=(const FatFile &) = delete;
  ~FatFile() { Free(); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  const auto &Archs() const { return arches; }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{bela::SizeUnInitialized};
  std::vector<FatArch> arches;
  bool needClosed{false};
};

} // namespace hazel::macho

#endif