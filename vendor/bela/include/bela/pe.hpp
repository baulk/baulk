///
#ifndef BELA_PE_HPP
#define BELA_PE_HPP
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <span>
#include "base.hpp"
#include "types.hpp"
#include "ascii.hpp"
#include "match.hpp"
#include "os.hpp"
#include "io.hpp"
#include "buffer.hpp"
#include "__windows/image.hpp"

namespace bela::pe {
// PE File resolve
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
class File {
private:
  bool parseFile(bela::error_code &ec);
  std::string sectionFullName(SectionHeader32 &sh) const;
  const DataDirectory *getDataDirectory(uint32_t dirIndex) const {
    if (dirIndex >= DataDirEntries) {
      return nullptr;
    }
    // check that the length of data directory entries is large
    // enough to include the dir directory.
    if (oh.NumberOfRvaAndSizes < dirIndex + 1) {
      return nullptr;
    }
    return &oh.DataDirectory[dirIndex];
  }
  // getSection figure out which section contains the 'dd' directory table
  const Section *getSection(const DataDirectory *dd) const {
    // Also, do not assume that the RVAs in this table point to the beginning of a section or that the sections that
    // contain specific tables have specific names.
    for (const auto &s : sections) {
      if (s.VirtualAddress <= dd->VirtualAddress && dd->VirtualAddress < s.VirtualAddress + s.VirtualSize) {
        return &s;
      }
    }
    return nullptr;
  }
  // getSection by name
  const Section *getSection(std::string_view name) const {
    for (const auto &s : sections) {
      if (s.Name == name) {
        return &s;
      }
    }
    return nullptr;
  }
  std::optional<Buffer> readSectionData(const Section &sec, bela::error_code &ec) const;
  bool readCOFFSymbols(std::vector<COFFSymbol> &symbols, bela::error_code &ec) const;
  bool readRelocs(Section &sec) const;
  bool readStringTable(bela::error_code &ec);
  bool LookupDelayImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;
  bool LookupImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;

public:
  File() = default;
  ~File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  template <typename AStringT> void SplitStringTable(std::vector<AStringT> &sa) const {
    auto sv = stringTable.buffer.as_bytes_view().make_string_view();
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
  // PE File size
  int64_t Size() const { return size; }
  int64_t OverlayOffset() const { return overlayOffset; }
  int64_t OverlayLength() const { return size - overlayOffset; }
  int64_t ReadOverlay(std::span<uint8_t> overlayData, bela::error_code &ec) const;
  bool LookupExports(std::vector<ExportedSymbol> &exports, bela::error_code &ec) const;
  bool LookupFunctionTable(FunctionTable &ft, bela::error_code &ec) const;
  bool LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const;
  std::optional<DotNetMetadata> LookupDotNetMetadata(bela::error_code &ec) const;
  std::optional<Version> LookupVersion(bela::error_code &ec) const; // WIP
  const FileHeader &Fh() const { return fh; }
  const auto &Header() const { return oh; }
  const auto &Sections() const { return sections; }
  bool Is64Bit() const { return oh.Is64Bit; }
  bela::pe::Machine Machine() const { return static_cast<bela::pe::Machine>(fh.Machine); }
  bela::pe::Subsystem Subsystem() const { return static_cast<bela::pe::Subsystem>(oh.Subsystem); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  //
  const auto &FD() const { return fd; }

private:
  bela::io::FD fd;
  int64_t size{SizeUnInitialized};
  FileHeader fh;
  OptionalHeader oh;
  std::vector<Section> sections;
  StringTable stringTable;
  int64_t overlayOffset{SizeUnInitialized};
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

// https://docs.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfoexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-getfileversioninfosizeexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-verqueryvaluew
// version.lib
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