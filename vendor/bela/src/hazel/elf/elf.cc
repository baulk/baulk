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
  if (size == bela::SizeUnInitialized) {
    LARGE_INTEGER li;
    if (GetFileSizeEx(fd, &li) != TRUE) {
      ec = bela::make_system_error_code(L"GetFileSizeEx: ");
      return false;
    }
    size = li.QuadPart;
  }
  uint8_t ident[16];
  if (!ReadAt(ident, sizeof(ident), 0, ec)) {
    return false;
  }
  constexpr uint8_t elfmagic[4] = {'\x7f', 'E', 'L', 'F'};
  if (memcmp(ident, elfmagic, sizeof(elfmagic)) != 0) {
    ec = bela::make_error_code(ErrGeneral, L"elf: bad magic number ['", static_cast<int>(ident[0]), L"', '",
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
    ec = bela::make_error_code(ErrGeneral, L"unkonw ELF class ", static_cast<int>(fh.Class));
    return false;
  }
  fh.Data = ident[EI_DATA];
  switch (fh.Data) {
  case ELFDATA2LSB:
    en = std::endian::little;
    break;
  case ELFDATA2MSB:
    en = std::endian::big;
    break;
  default:
    ec = bela::make_error_code(ErrGeneral, L"unkonw ELF data encoding ", static_cast<int>(fh.Data));
    return false;
  }
  fh.Version = ident[EI_VERSION];
  fh.ABI = ident[EI_OSABI];
  fh.ABIVersion = ident[EI_ABIVERSION];
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
    fh.Type = endian_cast(hdr.e_type);
    fh.Machine = endian_cast(hdr.e_machine);
    fh.Entry = endian_cast(hdr.e_entry);
    if (auto version = endian_cast(hdr.e_version); version != static_cast<uint32_t>(fh.Version)) {
      ec = bela::make_error_code(ErrGeneral, L"mismatched ELF version, got ", version, L" want ",
                                 static_cast<uint32_t>(fh.Version));
      return false;
    }
    phoff = endian_cast(hdr.e_phoff);
    phentsize = endian_cast(hdr.e_phentsize);
    phnum = endian_cast(hdr.e_phnum);
    shoff = endian_cast(hdr.e_shoff);
    shentsize = endian_cast(hdr.e_shentsize);
    shnum = endian_cast(hdr.e_shnum);
    shstrndx = endian_cast(hdr.e_shstrndx);
  } break;
  case ELFCLASS64: {
    Elf64_Ehdr hdr;
    if (!ReadAt(&hdr, sizeof(hdr), 0, ec)) {
      return false;
    }
    fh.Type = endian_cast(hdr.e_type);
    fh.Machine = endian_cast(hdr.e_machine);
    fh.Entry = endian_cast(hdr.e_entry);
    if (auto version = endian_cast(hdr.e_version); version != static_cast<uint32_t>(fh.Version)) {
      ec = bela::make_error_code(ErrGeneral, L"mismatched ELF version, got ", version, L" want ",
                                 static_cast<uint32_t>(fh.Version));
      return false;
    }
    phoff = endian_cast(hdr.e_phoff);
    phentsize = endian_cast(hdr.e_phentsize);
    phnum = endian_cast(hdr.e_phnum);
    shoff = endian_cast(hdr.e_shoff);
    shentsize = endian_cast(hdr.e_shentsize);
    shnum = endian_cast(hdr.e_shnum);
    shstrndx = endian_cast(hdr.e_shstrndx);
  } break;
  default:
    break;
  }
  //
  if (shoff == 0 && shnum != 0) {
    ec = bela::make_error_code(ErrGeneral, L"invalid ELF shnum for shoff=0 shnum=", shnum);
    return false;
  }
  if (shnum > 0 && shstrndx >= shnum) {
    ec = bela::make_error_code(ErrGeneral, L"invalid ELF shstrndx ", shstrndx);
    return false;
  }
  if (phnum < 0 || phnum > 10000) {
    ec = bela::make_error_code(ErrGeneral, L"invalid ELF phnum ", phnum);
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
      p->Type = endian_cast(ph.p_type);
      p->Flags = endian_cast(ph.p_flags);
      p->Off = endian_cast(ph.p_offset);
      p->Vaddr = endian_cast(ph.p_vaddr);
      p->Paddr = endian_cast(ph.p_paddr);
      p->Filesz = endian_cast(ph.p_filesz);
      p->Memsz = endian_cast(ph.p_memsz);
      p->Align = endian_cast(ph.p_align);
    } else {
      Elf64_Phdr ph;
      if (!ReadAt(&ph, sizeof(ph), off, ec)) {
        return false;
      }
      p->Type = endian_cast(ph.p_type);
      p->Flags = endian_cast(ph.p_flags);
      p->Off = endian_cast(ph.p_offset);
      p->Vaddr = endian_cast(ph.p_vaddr);
      p->Paddr = endian_cast(ph.p_paddr);
      p->Filesz = endian_cast(ph.p_filesz);
      p->Memsz = endian_cast(ph.p_memsz);
      p->Align = endian_cast(ph.p_align);
    }
  }
  if (shnum == 0) {
    return true;
  }
  if (shnum < 0 || shnum > 10000) {
    ec = bela::make_error_code(ErrGeneral, L"invalid ELF shnum ", shnum);
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
      p->Type = endian_cast(sh.sh_type);
      p->Flags = endian_cast(sh.sh_flags);
      p->Addr = endian_cast(sh.sh_addr);
      p->Offset = endian_cast(sh.sh_offset);
      p->FileSize = endian_cast(sh.sh_size);
      p->Link = endian_cast(sh.sh_link);
      p->Info = endian_cast(sh.sh_info);
      p->Addralign = endian_cast(sh.sh_addralign);
      p->Entsize = endian_cast(sh.sh_entsize);
      p->nameIndex = endian_cast(sh.sh_name);
    } else {
      Elf64_Shdr sh;
      // constexpr auto n=sizeof(Elf64_Shdr);
      if (!ReadAt(&sh, sizeof(sh), off, ec)) {
        return false;
      }
      p->Type = endian_cast(sh.sh_type);
      p->Flags = endian_cast(sh.sh_flags);
      p->Addr = endian_cast(sh.sh_addr);
      p->Offset = endian_cast(sh.sh_offset);
      p->FileSize = endian_cast(sh.sh_size);
      p->Link = endian_cast(sh.sh_link);
      p->Info = endian_cast(sh.sh_info);
      p->Addralign = endian_cast(sh.sh_addralign);
      p->Entsize = endian_cast(sh.sh_entsize);
      p->nameIndex = endian_cast(sh.sh_name);
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
      p->compressionType = endian_cast(ch.ch_type);
      p->Size = endian_cast(ch.ch_size);
      p->Addralign = endian_cast(ch.ch_addralign);
      p->compressionOffset = sizeof(ch);
    } else {
      Elf64_Chdr ch;
      if (!ReadAt(&ch, sizeof(ch), off, ec)) {
        return false;
      }
      p->compressionType = endian_cast(ch.ch_type);
      p->Size = endian_cast(ch.ch_size);
      p->Addralign = endian_cast(ch.ch_addralign);
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