//////// GIT pack index and other files.
#include "hazelinc.hpp"

namespace hazel::internal {
// todo resolve git index pack and midx files
#pragma pack(1)
struct git_pack_header_t {
  uint8_t signature[4]; /// P A C K
  uint32_t version;     // BE
  uint32_t objsize;     // BE
};

struct git_index_header_t {
  uint8_t siganture[4];
  uint32_t version;
  uint32_t fanout[256];
};

struct git_index3_header_t {
  uint8_t signature[4]; // 0xFF, 0x74, 0x4F, 0x63
  uint32_t version;
  uint32_t hslength;
  uint32_t packobjects;
  uint32_t objectformats; // 2
};

struct git_midx_header_t {
  uint8_t siganture[4]; // M I D X
  uint8_t version;
  uint8_t oidversion; // 1 SHA1
  uint8_t chunks;
  uint8_t basemidxsize; // 0
  uint32_t packfiles;   //
};
#pragma pack()
// https://github.com/git/git/blob/master/Documentation/technical/pack-format.txt
status_t explore_git_file(bela::MemView mv, FileAttributeTable &fat) {
  constexpr const uint8_t packMagic[] = {'P', 'A', 'C', 'K'};
  constexpr const uint8_t midxMagic[] = {'M', 'I', 'D', 'X'};
  constexpr const uint8_t indexMagic[] = {0xFF, 0x74, 0x4F, 0x63};
  if (mv.StartsWith(packMagic)) {
    auto hd = mv.cast<git_pack_header_t>(0);
    if (hd == nullptr) {
      return None;
    }
    auto name = bela::StringCat(L"Git pack file, version ", bela::swapbe(hd->version), L", objects ",
                                bela::swapbe(hd->objsize));
    fat.assign(std::move(name), types::gitpack);
    return Found;
  }
  if (mv.StartsWith(indexMagic)) {
    auto hd = mv.cast<git_index_header_t>(0);
    if (hd == nullptr) {
      return None;
    }
    std::wstring name;
    auto ver = bela::swapbe(hd->version);
    switch (ver) {
    case 2:
      name =
          bela::StringCat(L"Git pack indexs file, version ", ver, L", total objects ", bela::swapbe(hd->fanout[255]));
      break;
    case 3: {
      auto hd3 = mv.cast<git_index3_header_t>(0);
      name =
          bela::StringCat(L"Git pack indexs file, version ", ver, L", total objects ", bela::swapbe(hd3->packobjects));
    } break;
    default:
      name = bela::StringCat(L"Git pack indexs file, version ", ver);
      break;
    };

    fat.assign(std::move(name), types::gitpkindex);
    return Found;
  }
  if (mv.StartsWith(midxMagic)) {
    auto hd = mv.cast<git_midx_header_t>(0);
    if (hd == nullptr) {
      return None;
    }
    fat.assign(bela::StringCat(L"Git multi-pack-index, version ", (int)hd->version, L", oid version ",
                               (int)hd->oidversion, L", chunks ", (int)hd->chunks, L", pack files ",
                               bela::swapbe(hd->packfiles)),
               types::gitpack);
    return Found;
  }

  return None;
}

} // namespace hazel::internal
