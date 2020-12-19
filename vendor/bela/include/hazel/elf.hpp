//
#ifndef HAZEL_ELF_HPP
#define HAZEL_ELF_HPP
#include <bela/endian.hpp>
#include "hazel.hpp"
#include "details/elf.h"

namespace hazel::elf {

constexpr int COMPRESS_ZLIB = 1;            /* ZLIB compression. */
constexpr int COMPRESS_LOOS = 0x60000000;   /* First OS-specific. */
constexpr int COMPRESS_HIOS = 0x6fffffff;   /* Last OS-specific. */
constexpr int COMPRESS_LOPROC = 0x70000000; /* First processor-specific type. */
constexpr int COMPRESS_HIPROC = 0x7fffffff; /* Last processor-specific type. */

// ELF64 file header.
struct FileHeader {
  uint8_t Class;
  uint8_t Data;
  uint8_t Version;
  uint8_t OSABI;
  uint8_t ABIVersion;
  uint8_t LSB;
  uint16_t Type;    /* File type. */
  uint16_t Machine; /* Machine architecture. */
  uint64_t Entry;   /* Entry point. */
};

struct ProgHeader {
  int Type;
  int Flags;
  uint64_t Off;
  uint64_t Vaddr;
  uint64_t Paddr;
  uint64_t Filesz;
  uint64_t Memsz;
  uint64_t Align;
};

struct Symbol {
  std::string Name;
  std::string Version;
  std::string Library;
  uint64_t Value;
  uint64_t Size;
  int SectionIndex;
  uint8_t Info;
  uint8_t Other;
};

struct ImportedSymbol {
  std::string Name;
  std::string Version;
  std::string Library;
};

struct Section {
  std::string Name;
  uint32_t Type{0};
  uint64_t Flags{0};
  uint64_t Addr{0};
  uint64_t Offset{0};
  uint64_t Size{0};
  uint32_t Link{0};
  uint32_t Info{0};
  uint64_t Addralign{0};
  uint64_t Entsize{0};
  uint64_t FileSize{0};
  int64_t compressionOffset{0};
  int compressionType{0};
  uint32_t nameIndex;
};

struct verneed {
  std::string file;
  std::string name;
};

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
    needClosed = r.needClosed;
    r.needClosed = false;
    size = r.size;
    r.size = 0;
    sections = std::move(r.sections);
    progs = std::move(r.progs);
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
    if (en == std::endian::native) {
      return *reinterpret_cast<const Integer *>(p);
    }
    return bela::bswap(*reinterpret_cast<const Integer *>(p));
  }
  const Section *SectionByType(uint32_t st) const {
    for (const auto &s : sections) {
      if (s.Type == st) {
        return &s;
      }
    }
    return nullptr;
  }
  bool sectionData(const Section &sec, bela::Buffer &buffer, bela::error_code &ec) {
    buffer.grow(sec.Size);
    return ReadAt(buffer, sec.Size, sec.Offset, ec);
  }
  bool stringTable(uint32_t link, bela::Buffer &buf, bela::error_code &ec) {
    if (link <= 0 || link >= static_cast<uint32_t>(sections.size())) {
      ec = bela::make_error_code(L"section has invalid string table link");
      return false;
    }
    return sectionData(sections[link], buf, ec);
  }
  bool gnuVersionInit(std::span<uint8_t> str);
  void gnuVersion(int i, std::string &lib, std::string &ver) {
    i = (i + 1) * 2;
    if (i >= static_cast<int>(gnuVersym.size())) {
      return;
    }
    auto j = static_cast<size_t>(cast_from<uint16_t>(gnuVersym.data() + i));
    if (j < 2 || j >= gnuNeed.size()) {
      return;
    }
    lib = gnuNeed[j].file;
    ver = gnuNeed[j].name;
  }
  bool getSymbols64(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec);
  bool getSymbols32(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec);
  bool getSymbols(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) {
    if (is64bit) {
      return getSymbols64(st, syms, strdata, ec);
    }
    return getSymbols32(st, syms, strdata, ec);
  }

public:
  File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  ~File() { Free(); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  bool Is64Bit() const { return is64bit; }
  int64_t Size() const { return size; }
  const auto &Sections() const { return sections; }
  const auto &Progs() const { return progs; }
  bool DynString(int tag, std::vector<std::string> &sv, bela::error_code &ec);
  bool DynamicSymbols(std::vector<Symbol> &syms, bela::error_code &ec);
  bool ImportedSymbols(std::vector<ImportedSymbol> &symbols, bela::error_code &ec);
  bool Symbols(std::vector<Symbol> &syms, bela::error_code &ec) {
    bela::Buffer strdata;
    return getSymbols(SHT_SYMTAB, syms, strdata, ec);
  }
  // depend libs
  bool Depends(std::vector<std::string> &libs, bela::error_code &ec) { return DynString(DT_NEEDED, libs, ec); }
  std::optional<std::string> LibSoName(bela::error_code &ec) {
    std::vector<std::string> so;
    if (!DynString(DT_SONAME, so, ec)) {
      return std::nullopt;
    }
    if (so.empty()) {
      ec = bela::make_error_code(L"elf no soname");
      return std::nullopt;
    }
    return std::make_optional(std::move(so.front()));
  };

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{bela::SizeUnInitialized};
  std::endian en{std::endian::native};
  FileHeader fh;
  std::vector<Section> sections;
  std::vector<ProgHeader> progs;
  std::vector<verneed> gnuNeed;
  bela::Buffer gnuVersym;
  bool is64bit{false};
  bool needClosed{false};
};
} // namespace hazel::elf

#endif