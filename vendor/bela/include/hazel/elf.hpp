//
#ifndef HAZEL_ELF_HPP
#define HAZEL_ELF_HPP
#include <bela/endian.hpp>
#include "hazel.hpp"
#include "details/elf.h"

namespace hazel::elf {
using namespace llvm::ELF;
constexpr int COMPRESS_ZLIB = 1;            /* ZLIB compression. */
constexpr int COMPRESS_LOOS = 0x60000000;   /* First OS-specific. */
constexpr int COMPRESS_HIOS = 0x6fffffff;   /* Last OS-specific. */
constexpr int COMPRESS_LOPROC = 0x70000000; /* First processor-specific type. */
constexpr int COMPRESS_HIPROC = 0x7fffffff; /* Last processor-specific type. */

// ELF64 file header.
struct FileHeader {
  uint64_t Entry;   /* Entry point. */
  uint16_t Type;    /* File type. */
  uint16_t Machine; /* Machine architecture. */
  // magic 4 bytes
  uint8_t Class;
  uint8_t Data;
  uint8_t Version;
  uint8_t ABI;
  uint8_t ABIVersion;
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
  bool parseFile(bela::error_code &ec);
  void MoveFrom(File &&r) {
    fd = std::move(r.fd);
    size = r.size;
    r.size = 0;
    sections = std::move(r.sections);
    progs = std::move(r.progs);
    memcpy(&fh, &r.fh, sizeof(fh));
    memset(&r.fh, 0, sizeof(r.fh));
  }

  template <typename I>
    requires std::integral<I>
  I endian_cast(I t) const {
    if (en == std::endian::native) {
      return t;
    }
    return bela::bswap(t);
  }
  template <typename I>
    requires std::integral<I>
  I cast_from(const void *p) const {
    auto v = bela::unaligned_load<I>(p);
    if (en == std::endian::native) {
      return v;
    }
    return bela::bswap(v);
  }
  const Section *SectionByType(uint32_t st) const {
    for (const auto &s : sections) {
      if (s.Type == st) {
        return &s;
      }
    }
    return nullptr;
  }
  bool sectionData(const Section &sec, bela::Buffer &buffer, bela::error_code &ec) const {
    if (bela::narrow_cast<int64_t>(sec.Offset + sec.Size) > size) {
      ec = bela::make_error_code(bela::ErrFileTooSmall, L"corrupted ELF file, section overflow file: ", size,
                                 L" section end: ", sec.Offset + sec.Size);
      return false;
    }
    buffer.grow(static_cast<size_t>(sec.Size));
    if (!fd.ReadAt(buffer, static_cast<size_t>(sec.Size), sec.Offset, ec)) {
      return false;
    }
    return true;
  }

  bool stringTable(uint32_t link, bela::Buffer &buf, bela::error_code &ec) const {
    if (link <= 0 || link >= static_cast<uint32_t>(sections.size())) {
      ec = bela::make_error_code(L"section has invalid string table link");
      return false;
    }
    return sectionData(sections[link], buf, ec);
  }
  bool gnuVersionInit(std::span<const uint8_t> str);
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
  bool getSymbols64(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) const;
  bool getSymbols32(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) const;
  bool getSymbols(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) const {
    if (is64bit) {
      return getSymbols64(st, syms, strdata, ec);
    }
    return getSymbols32(st, syms, strdata, ec);
  }

public:
  File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  ~File() = default;
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  bool Is64Bit() const { return is64bit; }
  int64_t Size() const { return size; }
  const auto &Sections() const { return sections; }
  const auto &Progs() const { return progs; }
  const auto &Fh() const { return fh; }
  bool DynString(int tag, std::vector<std::string> &sv, bela::error_code &ec) const;
  std::optional<std::string> DynString(int tag, bela::error_code &ec) const {
    std::vector<std::string> so;
    if (!DynString(tag, so, ec)) {
      return std::nullopt;
    }
    if (so.empty()) {
      ec = bela::make_error_code(ErrGeneral, L"DynString: 0x", bela::Hex(tag), L" not exists");
      return std::nullopt;
    }
    return std::make_optional(std::move(so.front()));
  }
  bool DynamicSymbols(std::vector<Symbol> &syms, bela::error_code &ec);
  bool ImportedSymbols(std::vector<ImportedSymbol> &symbols, bela::error_code &ec);
  bool Symbols(std::vector<Symbol> &syms, bela::error_code &ec) const {
    bela::Buffer strdata;
    return getSymbols(SHT_SYMTAB, syms, strdata, ec);
  }
  // depend libs
  bool Depends(std::vector<std::string> &libs, bela::error_code &ec) { return DynString(DT_NEEDED, libs, ec); }
  std::optional<std::string> LibSoName(bela::error_code &ec) const { return DynString(DT_SONAME, ec); };
  // https://www.qt.io/blog/2011/10/28/rpath-and-runpath
  // https://wiki.debian.org/RpathIssue
  std::optional<std::string> Rpath(bela::error_code &ec) const { return DynString(DT_RPATH, ec); };
  std::optional<std::string> Rupath(bela::error_code &ec) const { return DynString(DT_RPATH, ec); };

private:
  bela::io::FD fd;
  int64_t size{bela::SizeUnInitialized};
  std::endian en{std::endian::native};
  FileHeader fh;
  std::vector<Section> sections;
  std::vector<ProgHeader> progs;
  std::vector<verneed> gnuNeed;
  bela::Buffer gnuVersym;
  bool is64bit{false};
};
} // namespace hazel::elf

#endif