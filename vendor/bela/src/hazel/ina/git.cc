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
status_t explore_git_file(bela::MemView mv, hazel_result &hr) {
  constexpr const uint8_t packMagic[] = {'P', 'A', 'C', 'K'};
  constexpr const uint8_t midxMagic[] = {'M', 'I', 'D', 'X'};
  constexpr const uint8_t indexMagic[] = {0xFF, 0x74, 0x4F, 0x63};
  if (mv.StartsWith(packMagic)) {
    git_pack_header_t hdr;
    auto hd = mv.bit_cast<git_pack_header_t>(&hdr);
    if (hd == nullptr) {
      return None;
    }
    hr.assign(types::gitpack, L"Git pack file");
    hr.append(L"Version", bela::frombe(hd->version));
    hr.append(L"Counts", bela::frombe(hd->objsize));
    return Found;
  }
  if (mv.StartsWith(indexMagic)) {
    git_index_header_t hdr;
    auto hd = mv.bit_cast<git_index_header_t>(&hdr);
    if (hd == nullptr) {
      return None;
    }
    hr.assign(types::gitpkindex, L"Git pack indexs file");
    auto version = bela::frombe(hd->version);
    hr.append(L"Version", version);
    switch (version) {
    case 2:
      hr.append(L"Counts", bela::frombe(hd->fanout[255]));
      break;
    case 3: {
      git_index3_header_t hdr_;
      if (auto hdr3 = mv.bit_cast<git_index3_header_t>(&hdr_); hdr3 != nullptr) {
        hr.append(L"Counts", bela::frombe(hdr3->packobjects));
      }
    } break;
    default:
      break;
    };
    return Found;
  }
  if (mv.StartsWith(midxMagic)) {
    git_midx_header_t hdr;
    auto hd = mv.bit_cast<git_midx_header_t>(&hdr);
    if (hd == nullptr) {
      return None;
    }
    hr.assign(types::gitmidx, L"Git multi-pack-index");
    hr.append(L"Version", static_cast<int>(hd->version));
    hr.append(L"OidVersion", static_cast<int>(hd->oidversion));
    hr.append(L"Chunks", static_cast<int>(hd->chunks));
    hr.append(L"Packfiles", bela::frombe(hd->packfiles));
    return Found;
  }

  return None;
}

} // namespace hazel::internal
