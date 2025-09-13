///
#include <hazel/elf.hpp>
#include <utility>

namespace hazel::elf {
bool File::gnuVersionInit(std::span<const uint8_t> str) {
  if (!gnuNeed.empty()) {
    // Already initialized
    return true;
  }
  bela::bytes_view bv(str);
  auto vn = SectionByType(SHT_GNU_verneed);
  if (vn == nullptr) {
    return false;
  }
  bela::error_code ec;
  bela::Buffer d;
  if (!sectionData(*vn, d, ec)) {
    return false;
  }
  int i = 0;
  auto sz = static_cast<int>(d.size());
  for (;;) {
    if (i + 16 > sz) {
      break;
    }
    if (auto vers = cast_from<uint16_t>(d.data() + i); vers != 1) {
      break;
    }
    auto cnt = static_cast<int>(cast_from<uint16_t>(d.data() + i + 2));
    auto fileoff = cast_from<uint32_t>(d.data() + i + 4);
    auto aux = cast_from<uint32_t>(d.data() + i + 8);
    auto next = cast_from<uint32_t>(d.data() + i + 12);
    auto file = bv.make_cstring_view(fileoff);
    auto j = i + static_cast<int>(aux);
    for (auto c = 0; c < cnt; c++) {
      if (j + 16 > sz) {
        break;
      }
      // uint32_t hash
      // uint16_t flags;
      auto ndx = static_cast<int>(cast_from<uint16_t>(d.data() + j + 6));
      auto nameoff = cast_from<uint32_t>(d.data() + j + 8);
      auto ndxNext = cast_from<uint32_t>(d.data() + j + 12);
      auto name = bv.make_cstring_view(nameoff);
      if (static_cast<size_t>(ndx) >= gnuNeed.size()) {
        gnuNeed.resize(2 * (ndx + 1));
      }
      gnuNeed[ndx].file = file;
      gnuNeed[ndx].name = name;
      if (ndxNext == 0) {
        break;
      }
      j += static_cast<int>(ndxNext);
    }
    if (next == 0) {
      break;
    }
    i += static_cast<int>(next);
  }
  auto vs = SectionByType(SHT_GNU_versym);
  if (vs == nullptr) {
    return false;
  }
  return sectionData(*vs, gnuVersym, ec);
}

bool File::DynamicSymbols(std::vector<Symbol> &syms, bela::error_code &ec) {
  bela::Buffer strdata;
  if (!getSymbols(SHT_DYNSYM, syms, strdata, ec)) {
    return false;
  }
  if (gnuVersionInit(strdata.make_const_span())) {
    for (int i = 0; std::cmp_less(i, syms.size()); i++) {
      gnuVersion(i, syms[i].Library, syms[i].Version);
    }
  }
  return true;
}

constexpr int SymBind(int i) { return i >> 4; }

bool File::ImportedSymbols(std::vector<ImportedSymbol> &symbols, bela::error_code &ec) {
  bela::Buffer strdata;
  std::vector<Symbol> syms;
  if (!getSymbols(SHT_DYNSYM, syms, strdata, ec)) {
    return false;
  }
  gnuVersionInit(strdata.make_const_span());
  symbols.reserve(syms.size());
  for (int i = 0; std::cmp_less(i, syms.size()); i++) {
    const auto &s = syms[i];
    if (SymBind(s.Info) == STB_GLOBAL && s.SectionIndex == SHN_UNDEF) {
      ImportedSymbol is;
      is.Name = s.Name;
      gnuVersion(i, is.Library, is.Version);
      symbols.emplace_back(std::move(is));
    }
  }
  return true;
}

} // namespace hazel::elf