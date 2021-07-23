///
#include <hazel/macho.hpp>

namespace hazel::macho {

bool FatFile::NewFile(std::wstring_view p, bela::error_code &ec) {
  if (fd) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  auto fd_ = bela::io::NewFile(p, ec);
  if (!fd_) {
    return false;
  }
  fd.Assgin(std::move(*fd_));
  return parseFile(ec);
}

bool FatFile::NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec) {
  if (fd) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd.Assgin(fd_, false);
  size = sz;
  return parseFile(ec);
}

//
bool FatFile::parseFile(bela::error_code &ec) {
  uint8_t ident[4] = {0};
  if (!fd.ReadAt(ident, 0, ec)) {
    return false;
  }
  if (bela::cast_frombe<uint32_t>(ident) != MagicFat) {
    // See if this is a Mach-O file via its magic number. The magic
    // must be converted to little endian first though.
    auto lm = bela::cast_fromle<uint32_t>(ident);
    if (lm == Magic32 || lm == Magic64) {
      ec = bela::make_error_code(ErrNotFat, L"not a fat Mach-O file");
      return false;
    }
    ec = bela::make_error_code(ErrGeneral, L"macho: bad magic number ['", static_cast<int>(ident[0]), L"', '",
                               static_cast<int>(ident[1]), L"', '", static_cast<int>(ident[2]), L"', '",
                               static_cast<int>(ident[3]), L"']");
    return false;
  }
  auto offset = 4ll;
  uint32_t narch{0};
  if (!fd.ReadFull(narch, ec)) {
    return false;
  }
  narch = bela::frombe(narch);
  uint32_t mt{0};
  offset += 4;
  if (narch < 1) {
    ec = bela::make_error_code(L"file contains no images");
    return false;
  }
  bela::flat_hash_map<uint64_t, bool> seenArches;
  arches.resize(narch);
  for (uint32_t i = 0; i < narch; i++) {
    auto p = &arches[i];
    fat_arch fa;
    if (fd.ReadFull(fa, ec)) {
      ec = bela::make_error_code(ec.code, L"invalid fat_arch header: ", ec.message);
      return false;
    }
    fa.align = bela::frombe(fa.align);
    fa.cpusubtype = bela::frombe(fa.cpusubtype);
    fa.cputype = bela::frombe(fa.cputype);
    fa.offset = bela::frombe(fa.offset);
    fa.size = bela::frombe(fa.size);
    offset += sizeof(fa);
    p->file.baseOffset = fa.offset;
    if (!p->file.NewFile(fd.NativeFD(), p->file.size, ec)) {
      return false;
    }
    auto seenArch = (static_cast<uint64_t>(fa.cputype) << 32) | static_cast<uint64_t>(fa.cpusubtype);
    if (auto it = seenArches.find(seenArch); it != seenArches.end()) {
      ec = bela::make_error_code(ErrGeneral, L"duplicate architecture cpu=", fa.cputype, L", subcpu=#",
                                 bela::AlphaNum(bela::Hex(fa.cpusubtype)));
      return false;
    }
    seenArches[seenArch] = true;
    auto machoType = p->file.fh.Type;
    if (i == 0) {
      mt = machoType;
      continue;
    }
    if (machoType != mt) {
      ec = bela::make_error_code(ErrGeneral, L"Mach-O type for architecture #", i, L" (type=#",
                                 bela::AlphaNum(bela::Hex(machoType)), L") does not match first (type=#",
                                 bela::AlphaNum(bela::Hex(mt)), L")");
      return false;
    }
  }
  return true;
}

} // namespace hazel::macho