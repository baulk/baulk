//
#include <hazel/macho.hpp>

namespace hazel::macho {
//

bool File::NewFile(std::wstring_view p, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = CreateFileW(p.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  needClosed = true;
  return ParseFile(ec);
}
bool File::NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = fd_;
  size = sz;
  return ParseFile(ec);
}

bool File::readFileHeader(int64_t &offset, bela::error_code &ec) {
  uint8_t ident[4] = {0};
  if (!ReadAt(ident, sizeof(ident), 0, ec)) {
    return false;
  }
  auto le = bela::cast_fromle<uint32_t>(ident);
  if (le == MH_MAGIC) {
    en = std::endian::little;
    mach_header mh;
    if (!ReadAt(&mh, sizeof(mh), 0, ec)) {
      return false;
    }
    offset = sizeof(mach_header);
    fh.Magic = MH_MAGIC;
    fh.Cpu = endian_cast(mh.cputype);
    fh.SubCpu = endian_cast(mh.cpusubtype);
    fh.Type = endian_cast(mh.filetype);
    fh.Ncmd = endian_cast(mh.ncmds);
    fh.Cmdsz = endian_cast(mh.sizeofcmds);
    fh.Flags = endian_cast(mh.flags);
    return true;
  }
  if (le == MH_MAGIC_64) {
    en = std::endian::little;
    mach_header_64 mh;
    if (!ReadAt(&mh, sizeof(mh), 0, ec)) {
      return false;
    }
    offset = sizeof(mach_header_64);
    fh.Magic = MH_MAGIC_64;
    fh.Cpu = endian_cast(mh.cputype);
    fh.SubCpu = endian_cast(mh.cpusubtype);
    fh.Type = endian_cast(mh.filetype);
    fh.Ncmd = endian_cast(mh.ncmds);
    fh.Cmdsz = endian_cast(mh.sizeofcmds);
    fh.Flags = endian_cast(mh.flags);
    return true;
  }

  auto be = bela::cast_fromle<uint32_t>(ident);
  if (be == MH_MAGIC) {
    en = std::endian::big;
    mach_header mh;
    if (!ReadAt(&mh, sizeof(mh), 0, ec)) {
      return false;
    }
    offset = sizeof(mach_header);
    fh.Magic = MH_MAGIC;
    fh.Cpu = endian_cast(mh.cputype);
    fh.SubCpu = endian_cast(mh.cpusubtype);
    fh.Type = endian_cast(mh.filetype);
    fh.Ncmd = endian_cast(mh.ncmds);
    fh.Cmdsz = endian_cast(mh.sizeofcmds);
    fh.Flags = endian_cast(mh.flags);
    return true;
  }
  if (be == MH_MAGIC_64) {
    en = std::endian::big;
    mach_header_64 mh;
    if (!ReadAt(&mh, sizeof(mh), 0, ec)) {
      return false;
    }
    offset = sizeof(mach_header_64);
    fh.Magic = MH_MAGIC_64;
    fh.Cpu = endian_cast(mh.cputype);
    fh.SubCpu = endian_cast(mh.cpusubtype);
    fh.Type = endian_cast(mh.filetype);
    fh.Ncmd = endian_cast(mh.ncmds);
    fh.Cmdsz = endian_cast(mh.sizeofcmds);
    fh.Flags = endian_cast(mh.flags);
    return true;
  }
  ec = bela::make_error_code(1, L"macho: bad magic number ['", static_cast<int>(ident[0]), L"', '",
                             static_cast<int>(ident[1]), L"', '", static_cast<int>(ident[2]), L"', '",
                             static_cast<int>(ident[3]), L"']");
  return false;
}

inline std::string_view cstring(std::string_view str) {
  if (auto pos = str.find('\0'); pos != std::string_view::npos) {
    return str.substr(0, pos);
  }
  return str;
}

bool File::parseSymtab(std::string_view symdat, std::string_view strtab, std::string_view cmddat, const SymtabCmd &hdr,
                       int64_t offset, bela::error_code &ec) {
  symtab.Syms.resize(hdr.Nsyms);
  auto b = symdat;
  for (uint32_t i = 0; i < hdr.Nsyms; i++) {
    Nlist64 nl;
    if (is64bit) {
      if (b.size() < sizeof(Nlist64)) {
        ec = bela::make_error_code(L"unexpected EOF");
        return false;
      }
      auto p = reinterpret_cast<const Nlist64 *>(b.data());
      nl.Name = endian_cast(p->Name);
      nl.Type = endian_cast(p->Type);
      nl.Sect = endian_cast(p->Sect);
      nl.Desc = endian_cast(p->Desc);
      nl.Value = endian_cast(p->Value);
      b.remove_prefix(sizeof(Nlist64));
    } else {
      if (b.size() < sizeof(Nlist32)) {
        ec = bela::make_error_code(L"unexpected EOF");
        return false;
      }
      auto p = reinterpret_cast<const Nlist32 *>(b.data());
      nl.Name = endian_cast(p->Name);
      nl.Type = endian_cast(p->Type);
      nl.Sect = endian_cast(p->Sect);
      nl.Desc = endian_cast(p->Desc);
      nl.Value = endian_cast(p->Value);
      b.remove_prefix(sizeof(Nlist32));
    }
    auto sym = &symtab.Syms[i];
    if (nl.Name > static_cast<uint32_t>(strtab.size())) {
      ec = bela::make_error_code(L"invalid name in symbol table");
      return false;
    }
    auto name = cstring(strtab.substr(nl.Name));
    if (bela::StrContains(name, ".") && name[0] == '-') {
      name.remove_prefix(1);
    }
    sym->Name = name;
    sym->Type = nl.Type;
    sym->Sect = nl.Sect;
    sym->Desc = nl.Desc;
    sym->Value = nl.Value;
  }
  symtab.Cmd = hdr.Cmd;
  symtab.Len = hdr.Len;
  symtab.Nsyms = hdr.Nsyms;
  symtab.Stroff = hdr.Stroff;
  symtab.Strsize = hdr.Strsize;
  symtab.Symoff = hdr.Symoff;
  symtab.Bytes = cmddat;
  return true;
}
#pragma pack(4)
// struct relocInfo {
//   uint32_t Addr;
//   uint32_t Symnum;
// };
// #pragma pack()
bool File::pushSection(hazel::macho::Section *sh, bela::error_code &ec) {
  if (sh->Nreloc > 0) {
    bela::Buffer reldat(sh->Nreloc * 8);
    if (!ReadAt(reldat, sh->Nreloc * 8, sh->Reloff, ec)) {
      return false;
    }
    std::string_view b{reinterpret_cast<const char *>(reldat.data()), reldat.size()};
    sh->Relocs.resize(sh->Nreloc);
    for (uint32_t i = 0; i < sh->Nreloc; i++) {
      auto &rel = (sh->Relocs[i]);
      auto addr = cast_from<uint32_t>(b.data());
      auto symnum = cast_from<uint32_t>(b.data() + 4);
      b.remove_prefix(8);
      if ((addr & (1 << 31)) != 0) {
        rel.Addr = addr & ((1 << 24) - 1);
        rel.Type = static_cast<uint8_t>((addr >> 24) & ((1 << 4) - 1));
        rel.Len = static_cast<uint8_t>((addr >> 28) & ((1 << 2) - 1));
        rel.Pcrel = (addr & (1 << 30)) != 0;
        rel.Value = symnum;
        rel.Scattered = true;
        continue;
      }
      if (en == std::endian::little) {
        rel.Addr = addr;
        rel.Value = symnum & ((1 << 24) - 1);
        rel.Pcrel = (symnum & (1 << 24)) != 0;
        rel.Len = static_cast<uint8_t>((symnum >> 25) & ((1 << 2) - 1));
        rel.Extern = (symnum & (1 << 27)) != 0;
        rel.Type = static_cast<uint8_t>((symnum >> 28) & ((1 << 4) - 1));
        continue;
      }
      if (en == std::endian::big) {
        rel.Addr = addr;
        rel.Value = symnum >> 8;
        rel.Pcrel = (symnum & (1 << 7)) != 0;
        rel.Len = static_cast<uint8_t>((symnum >> 5) & ((1 << 2) - 1));
        rel.Extern = (symnum & (1 << 4)) != 0;
        rel.Type = static_cast<uint8_t>(symnum & ((1 << 4) - 1));
        continue;
      }
      ec = bela::make_error_code(L"unreachable");
      return false;
    }
  }
  return true;
}

bool File::ParseFile(bela::error_code &ec) {
  if (size == bela::SizeUnInitialized) {
    LARGE_INTEGER li;
    if (GetFileSizeEx(fd, &li) != TRUE) {
      ec = bela::make_system_error_code(L"GetFileSizeEx: ");
      return false;
    }
    size = li.QuadPart;
  }
  int64_t offset = {0};
  if (!readFileHeader(offset, ec)) {
    return false;
  }
  is64bit = (fh.Magic == Magic64);
  bela::Buffer buffer(fh.Cmdsz);
  if (!ReadAt(buffer, fh.Cmdsz, offset, ec)) {
    return false;
  }
  std::string_view dat{reinterpret_cast<const char *>(buffer.data()), fh.Cmdsz};
  loads.resize(fh.Ncmd);
  for (size_t i = 0; i < loads.size(); i++) {
    if (dat.size() < 8) {
      ec = bela::make_error_code(L"command block too small");
      return false;
    }
    auto cmd = cast_from<uint32_t>(dat.data());
    auto siz = cast_from<uint32_t>(dat.data() + 4);
    if (siz < 8 || siz > static_cast<uint32_t>(dat.size())) {
      ec = bela::make_error_code(L"invalid command block size");
      return false;
    }
    auto cmddat = dat.substr(0, siz);
    dat.remove_prefix(siz);
    offset += siz;
    loads[i].Cmd = cmd;
    switch (cmd) {
    case LoadCmdRpath: {
      if (cmddat.size() < 12) {
        ec = bela::make_error_code(L"invalid block in rpath command");
        return false;
      }
      RpathCmd hdr;
      hdr.Cmd = cast_from<uint32_t>(cmddat.data());
      hdr.Len = cast_from<uint32_t>(cmddat.data() + 4);
      hdr.Path = cast_from<uint32_t>(cmddat.data() + 8);
      if (hdr.Path >= static_cast<uint32_t>(cmddat.size())) {
        ec = bela::make_error_code(L"invalid path in rpath command");
        return false;
      }
      loads[i].Path = new std::string(cstring(cmddat.substr(hdr.Path)));
      loads[i].bytes = cmddat;
    } break;
    case LoadCmdDylib: {
      if (cmddat.size() < sizeof(DylibCmd)) {
        ec = bela::make_error_code(L"invalid block in dynamic library command");
        return false;
      }
      auto p = reinterpret_cast<const DylibCmd *>(cmddat.data());
      auto NameLen = endian_cast(p->Name);
      if (NameLen >= static_cast<uint32_t>(cmddat.size())) {
        ec = bela::make_error_code(L"invalid name in dynamic library command");
        return false;
      }
      loads[i].bytes = cmddat;
      loads[i].DyLib = new hazel::macho::DyLib();
      loads[i].DyLib->Name = cstring(cmddat.substr(NameLen));
      loads[i].DyLib->Time = endian_cast(p->Time);
      loads[i].DyLib->CurrentVersion = endian_cast(p->CurrentVersion);
      loads[i].DyLib->CompatVersion = endian_cast(p->CompatVersion);
    } break;
    case LoadCmdSymtab: {
      if (cmddat.size() < sizeof(SymtabCmd)) {
        ec = bela::make_error_code(L"invalid block in symtab command");
        return false;
      }
      auto p = reinterpret_cast<const SymtabCmd *>(cmddat.data());
      SymtabCmd hdr;
      hdr.Cmd = static_cast<LoadCmd>(cmd);
      hdr.Len = endian_cast(p->Len);
      hdr.Nsyms = endian_cast(p->Nsyms);
      hdr.Stroff = endian_cast(p->Stroff);
      hdr.Strsize = endian_cast(p->Strsize);
      hdr.Symoff = endian_cast(p->Symoff);
      bela::Buffer strtab(hdr.Strsize);
      if (!ReadAt(strtab, hdr.Strsize, hdr.Stroff, ec)) {
        return false;
      }
      auto symsz = 12;
      if (fh.Magic == Magic64) {
        symsz = 16;
      }
      auto symdatsz = hdr.Nsyms * symsz;
      bela::Buffer symdat(symdatsz);
      if (!ReadAt(symdat, symdatsz, hdr.Symoff, ec)) {
        return false;
      }
      std::string_view symdatsv{reinterpret_cast<const char *>(symdat.data()), symdat.size()};
      std::string_view strtabsv{reinterpret_cast<const char *>(strtab.data()), strtab.size()};
      if (!parseSymtab(symdatsv, strtabsv, cmddat, hdr, offset, ec)) {
        return false;
      }
    } break;
    case LoadCmdDysymtab: {
      if (cmddat.size() < sizeof(DysymtabCmd)) {
        ec = bela::make_error_code(L"invalid block in dysymtab command");
        return false;
      }
      auto p = reinterpret_cast<const DysymtabCmd *>(cmddat.data());
      dysymtab.Cmd = cmd;
      dysymtab.Len = endian_cast(p->Len);
      dysymtab.Ilocalsym = endian_cast(p->Ilocalsym);
      dysymtab.Nlocalsym = endian_cast(p->Nlocalsym);
      dysymtab.Iextdefsym = endian_cast(p->Iextdefsym);
      dysymtab.Nextdefsym = endian_cast(p->Nextdefsym);
      dysymtab.Iundefsym = endian_cast(p->Iundefsym);
      dysymtab.Nundefsym = endian_cast(p->Nundefsym);
      dysymtab.Tocoffset = endian_cast(p->Tocoffset);
      dysymtab.Ntoc = endian_cast(p->Ntoc);
      dysymtab.Modtaboff = endian_cast(p->Modtaboff);
      dysymtab.Nmodtab = endian_cast(p->Nmodtab);
      dysymtab.Extrefsymoff = endian_cast(p->Extrefsymoff);
      dysymtab.Nextrefsyms = endian_cast(p->Nextrefsyms);
      dysymtab.Indirectsymoff = endian_cast(p->Indirectsymoff);
      dysymtab.Nindirectsyms = endian_cast(p->Nindirectsyms);
      dysymtab.Extreloff = endian_cast(p->Extreloff);
      dysymtab.Nextrel = endian_cast(p->Nextrel);
      dysymtab.Locreloff = endian_cast(p->Locreloff);
      dysymtab.Nlocrel = endian_cast(p->Nlocrel);
      dysymtab.IndirectSyms.resize(dysymtab.Nindirectsyms);
      if (!ReadAt(dysymtab.IndirectSyms.data(), dysymtab.Nindirectsyms * 4, dysymtab.Indirectsymoff, ec)) {
        return false;
      }
      for (uint32_t j = 0; j < dysymtab.Nindirectsyms; j++) {
        dysymtab.IndirectSyms[j] = endian_cast(dysymtab.IndirectSyms[j]);
      }
    } break;
    case LoadCmdSegment: {
      if (cmddat.size() < sizeof(Segment32)) {
        ec = bela::make_error_code(L"invalid block in Segment command");
        return false;
      }
      auto p = reinterpret_cast<const Segment32 *>(cmddat.data());
      loads[i].Segment = new hazel::macho::Segment();
      auto s = loads[i].Segment;
      s->Bytes = cmddat;
      s->Cmd = cmd;
      s->Len = siz;
      s->Name = cstring({reinterpret_cast<const char *>(p->Name), sizeof(p->Name)});
      s->Addr = endian_cast(p->Addr);
      s->Memsz = endian_cast(p->Memsz);
      s->Offset = endian_cast(p->Offset);
      s->Filesz = endian_cast(p->Filesz);
      s->Maxprot = endian_cast(p->Maxprot);
      s->Prot = endian_cast(p->Prot);
      s->Nsect = endian_cast(p->Nsect);
      s->Flag = endian_cast(p->Flag);
      auto b = cmddat.substr(sizeof(Segment32));
      sections.resize(s->Nsect);
      for (uint32_t i = 0; i < s->Nsect; i++) {
        if (b.size() < sizeof(Section32)) {
          ec = bela::make_error_code(L"invalid block in Section32 data");
          return false;
        }
        auto se = reinterpret_cast<const Section32 *>(b.data());
        b.remove_prefix(sizeof(Section32));
        auto sh = &(sections[i]);
        sh->Name = cstring({reinterpret_cast<const char *>(se->Name), sizeof(se->Name)});
        sh->Seg = cstring({reinterpret_cast<const char *>(se->Seg), sizeof(se->Seg)});
        sh->Addr = endian_cast(se->Addr);
        sh->Size = endian_cast(se->Size);
        sh->Offset = endian_cast(se->Offset);
        sh->Align = endian_cast(se->Align);
        sh->Reloff = endian_cast(se->Reloff);
        sh->Nreloc = endian_cast(se->Nreloc);
        sh->Flags = endian_cast(se->Flags);
        if (!pushSection(sh, ec)) {
          return false;
        }
      }
    } break;
    case LoadCmdSegment64: {
      if (cmddat.size() < sizeof(Segment64)) {
        ec = bela::make_error_code(L"invalid block in Segment command");
        return false;
      }
      auto p = reinterpret_cast<const Segment64 *>(cmddat.data());
      loads[i].Segment = new hazel::macho::Segment();
      auto s = loads[i].Segment;
      s->Bytes = cmddat;
      s->Cmd = cmd;
      s->Len = siz;
      s->Name = cstring({reinterpret_cast<const char *>(p->Name), sizeof(p->Name)});
      s->Addr = endian_cast(p->Addr);
      s->Memsz = endian_cast(p->Memsz);
      s->Offset = endian_cast(p->Offset);
      s->Filesz = endian_cast(p->Filesz);
      s->Maxprot = endian_cast(p->Maxprot);
      s->Prot = endian_cast(p->Prot);
      s->Nsect = endian_cast(p->Nsect);
      s->Flag = endian_cast(p->Flag);
      auto b = cmddat.substr(sizeof(Segment64));
      sections.resize(s->Nsect);
      for (uint32_t j = 0; j < s->Nsect; j++) {
        if (b.size() < sizeof(Section64)) {
          ec = bela::make_error_code(L"invalid block in Section64 data");
          return false;
        }
        auto se = reinterpret_cast<const Section64 *>(b.data());
        b.remove_prefix(sizeof(Section64));
        auto sh = &(sections[j]);
        sh->Name = cstring({reinterpret_cast<const char *>(se->Name), sizeof(se->Name)});
        sh->Seg = cstring({reinterpret_cast<const char *>(se->Seg), sizeof(se->Seg)});
        sh->Addr = endian_cast(se->Addr);
        sh->Size = endian_cast(se->Size);
        sh->Offset = endian_cast(se->Offset);
        sh->Align = endian_cast(se->Align);
        sh->Reloff = endian_cast(se->Reloff);
        sh->Nreloc = endian_cast(se->Nreloc);
        sh->Flags = endian_cast(se->Flags);
        if (!pushSection(sh, ec)) {
          return false;
        }
      }
    } break;
    default:
      loads[i].bytes = cmddat;
      break;
    }
  }
  return true;
}
bool File::Depends(std::vector<std::string> &libs, bela::error_code &ec) {
  for (const auto &l : loads) {
    if (l.Cmd == LoadCmdDylib && l.DyLib != nullptr) {
      libs.emplace_back(l.DyLib->Name);
    }
  }
  return true;
}

bool File::ImportedSymbols(std::vector<std::string> &symbols, bela::error_code &ec) {
  if (dysymtab.Cmd == 0 || symtab.Cmd == 0) {
    ec = bela::make_error_code(L"missing symbol table");
    return false;
  }
  auto end = (std::min)(dysymtab.Iundefsym + dysymtab.Nundefsym, static_cast<uint32_t>(symtab.Syms.size()));
  for (uint32_t i = dysymtab.Iundefsym; i < end; i++) {
    symbols.emplace_back(symtab.Syms[i].Name);
  }
  return true;
}

} // namespace hazel::macho