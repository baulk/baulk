///
#include <hazel/hazel.hpp>
#include "internal.hpp"

namespace hazel::elf {
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

bool File::ParseFile(bela::error_code &ec) {
  uint8_t ident[16];
  constexpr auto x = sizeof(FileHeader);
  if (!ReadAt(ident, sizeof(ident), 0, ec)) {
    return false;
  }
  constexpr uint8_t elfmagic[4] = {'\x7f', 'E', 'L', 'F'};
  if (memcmp(ident, elfmagic, sizeof(elfmagic)) != 0) {
    ec = bela::make_error_code(1, L"elf: bad magic number ['", static_cast<int>(ident[0]), L"', '",
                               static_cast<int>(ident[1]), L"', '", static_cast<int>(ident[2]), L"', '",
                               static_cast<int>(ident[3]), L"']");
    return false;
  }
  fh.Class = ident[EI_CLASS];
  switch (fh.Class) {
  case ELFCLASS32:
    break;
  case ELFCLASS64:
    is64bit = true;
    break;
  default:
    ec = bela::make_error_code(1, L"unkonw ELF class ", static_cast<int>(fh.Class));
    return false;
  }
  fh.Data = ident[EI_DATA];
  switch (fh.Data) {
  case ELFDATA2LSB:
    en = bela::endian::Endian::little;
    break;
  case ELFDATA2MSB:
    en = bela::endian::Endian::big;
    break;
  default:
    ec = bela::make_error_code(1, L"unkonw ELF data encoding ", static_cast<int>(fh.Data));
    return false;
  }
  fh.Version = ident[EI_VERSION];
  fh.OSABI = ident[EI_OSABI];
  // buna/debug/elf/file.go
  int64_t phoff = 0;
  int phentsize = 0;
  int phnum = 0;
  int64_t shoff = 0;
  int shentsize = 0;
  int shnum = 0;
  int shstrndx = 0;
  switch (fh.Class) {
  case ELFCLASS32: {
    Elf32_Ehdr hdr;
    if (!ReadAt(&hdr, sizeof(hdr), 0, ec)) {
      return false;
    }
    fh.Type = EndianCast(hdr.e_type);
    fh.Machine = EndianCast(hdr.e_machine);
    fh.Entry = EndianCast(hdr.e_entry);
    if (auto version = EndianCast(hdr.e_version); version != static_cast<uint32_t>(fh.Version)) {
      ec = bela::make_error_code(1, L"mismatched ELF version, got ", version, L" want ",
                                 static_cast<uint32_t>(fh.Version));
      return false;
    }
    phoff = EndianCast(hdr.e_phoff);
    phentsize = EndianCast(hdr.e_phentsize);
    phnum = EndianCast(hdr.e_phnum);
    shoff = EndianCast(hdr.e_shoff);
    shentsize = EndianCast(hdr.e_shentsize);
    shnum = EndianCast(hdr.e_shnum);
    shstrndx = EndianCast(hdr.e_shstrndx);
  } break;
  case ELFCLASS64: {
    Elf64_Ehdr hdr;
    if (!ReadAt(&hdr, sizeof(hdr), 0, ec)) {
      return false;
    }
    fh.Type = EndianCast(hdr.e_type);
    fh.Machine = EndianCast(hdr.e_machine);
    fh.Entry = EndianCast(hdr.e_entry);
    if (auto version = EndianCast(hdr.e_version); version != static_cast<uint32_t>(fh.Version)) {
      ec = bela::make_error_code(1, L"mismatched ELF version, got ", version, L" want ",
                                 static_cast<uint32_t>(fh.Version));
      return false;
    }
    phoff = EndianCast(hdr.e_phoff);
    phentsize = EndianCast(hdr.e_phentsize);
    phnum = EndianCast(hdr.e_phnum);
    shoff = EndianCast(hdr.e_shoff);
    shentsize = EndianCast(hdr.e_shentsize);
    shnum = EndianCast(hdr.e_shnum);
    shstrndx = EndianCast(hdr.e_shstrndx);
  } break;
  default:
    break;
  }
  //
  if (shoff == 0 && shnum != 0) {
    ec = bela::make_error_code(1, L"invalid ELF shnum for shoff=0 shnum=", shnum);
    return false;
  }
  if (shnum > 0 && shstrndx >= shnum) {
    ec = bela::make_error_code(1, L"invalid ELF shstrndx ", shstrndx);
    return false;
  }
  if (phnum < 0 || phnum > 10000) {
    ec = bela::make_error_code(1, L"invalid ELF phnum ", phnum);
    return false;
  }
  progs.resize(phnum);
  for (auto i = 0; i < phnum; i++) {
    auto off = phoff + i * phentsize;
    auto p = &progs[i];
    if (fh.Class == ELFCLASS32) {
      Elf32_Phdr ph;
      if (!ReadAt(&ph, sizeof(ph), off, ec)) {
        return false;
      }
      p->Type = EndianCast(ph.p_type);
      p->Flags = EndianCast(ph.p_flags);
      p->Off = EndianCast(ph.p_offset);
      p->Vaddr = EndianCast(ph.p_vaddr);
      p->Paddr = EndianCast(ph.p_paddr);
      p->Filesz = EndianCast(ph.p_filesz);
      p->Memsz = EndianCast(ph.p_memsz);
      p->Align = EndianCast(ph.p_align);
    } else {
      Elf64_Phdr ph;
      if (!ReadAt(&ph, sizeof(ph), off, ec)) {
        return false;
      }
      p->Type = EndianCast(ph.p_type);
      p->Flags = EndianCast(ph.p_flags);
      p->Off = EndianCast(ph.p_offset);
      p->Vaddr = EndianCast(ph.p_vaddr);
      p->Paddr = EndianCast(ph.p_paddr);
      p->Filesz = EndianCast(ph.p_filesz);
      p->Memsz = EndianCast(ph.p_memsz);
      p->Align = EndianCast(ph.p_align);
    }
  }
  if (shnum == 0) {
    return true;
  }
  if (shnum < 0 || shnum > 10000) {
    ec = bela::make_error_code(1, L"invalid ELF shnum ", shnum);
    return false;
  }
  sections.resize(shnum);
  std::vector<int> names;
  for (auto i = 0; i < shnum; i++) {
    auto off = shoff + i * shentsize;
    auto p = &sections[i];
    if (fh.Class == ELFCLASS32) {
      Elf32_Shdr sh;
      if (!ReadAt(&sh, sizeof(sh), off, ec)) {
        return false;
      }
      p->Type = EndianCast(sh.sh_type);
      p->Flags = EndianCast(sh.sh_flags);
      p->Addr = EndianCast(sh.sh_addr);
      p->Offset = EndianCast(sh.sh_offset);
      p->FileSize = EndianCast(sh.sh_size);
      p->Link = EndianCast(sh.sh_link);
      p->Info = EndianCast(sh.sh_info);
      p->Addralign = EndianCast(sh.sh_addralign);
      p->Entsize = EndianCast(sh.sh_entsize);
      p->nameIndex = EndianCast(sh.sh_name);
    } else {
      Elf64_Shdr sh;
      // constexpr auto n=sizeof(Elf64_Shdr);
      if (!ReadAt(&sh, sizeof(sh), off, ec)) {
        return false;
      }
      p->Type = EndianCast(sh.sh_type);
      p->Flags = EndianCast(sh.sh_flags);
      p->Addr = EndianCast(sh.sh_addr);
      p->Offset = EndianCast(sh.sh_offset);
      p->FileSize = EndianCast(sh.sh_size);
      p->Link = EndianCast(sh.sh_link);
      p->Info = EndianCast(sh.sh_info);
      p->Addralign = EndianCast(sh.sh_addralign);
      p->Entsize = EndianCast(sh.sh_entsize);
      p->nameIndex = EndianCast(sh.sh_name);
    }
    if ((p->Flags & SHF_COMPRESSED) == 0) {
      p->Size = p->FileSize;
      continue;
    }
    if (fh.Class == ELFCLASS32) {
      Elf32_Chdr ch;
      if (!ReadAt(&ch, sizeof(ch), off, ec)) {
        return false;
      }
      p->compressionType = EndianCast(ch.ch_type);
      p->Size = EndianCast(ch.ch_size);
      p->Addralign = EndianCast(ch.ch_addralign);
      p->compressionOffset = sizeof(ch);
    } else {
      Elf64_Chdr ch;
      if (!ReadAt(&ch, sizeof(ch), off, ec)) {
        return false;
      }
      p->compressionType = EndianCast(ch.ch_type);
      p->Size = EndianCast(ch.ch_size);
      p->Addralign = EndianCast(ch.ch_addralign);
      p->compressionOffset = sizeof(ch);
    }
  }
  if (shstrndx < 0) {
    return false;
  }
  bela::Buffer buffer(sections[shstrndx].Size + 8);
  if (!sectionData(sections[shstrndx], buffer, ec)) {
    return false;
  }
  for (auto i = 0; i < shnum; i++) {
    sections[i].Name = getString(buffer.Span(), static_cast<int>(sections[i].nameIndex));
  }
  return true;
}

} // namespace hazel::elf