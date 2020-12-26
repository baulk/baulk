///////////////
#include "hazelinc.hpp"

namespace hazel::internal {

struct image_offset_detect {
  size_t offset;
  const char *magic;
  const wchar_t *mime;
  const wchar_t *desc;
  types::hazel_types_t t;
};

status_t LookupNewImages(bela::MemView mv, hazel_result &hr) {
  constexpr const image_offset_detect images[]{
      {8, "mif1", L"image/heif", L"HEIF Image", types::mif1},
      {8, "msf1", L"image/heif-sequence", L"HEIF Image Sequence", types::msf1},
      {8, "heic", L"image/heic", L"HEIF Image HEVC Main or Main Still Picture Profile", types::heic},
      {8, "heix", L"image/heic", L"HEIF Image HEVC Main 10 Profile", types::heix},
      {8, "hevc", L"image/heic-sequence", L"HEIF Image Sequenz HEVC Main or Main Still Picture Profile", types::hevc},
      {8, "hevx", L"image/heic-sequence", L"HEIF Image Sequence HEVC Main 10 Profile", types::hevx},
      // https://github.com/nokiatech/heif/blob/d5e9a21c8ba8df712bdf643021dd9f6518134776/Srcs/reader/hevcimagefilereader.cpp
      {8, "heim", L"image/heif", L"HEIF Image L-HEVC", types::heim},
      {8, "heis", L"image/heif", L"HEIF Image L-HEVC", types::heis},
      {8, "avic", L"image/heif", L"HEIF Image AVC", types::avic},
      {8, "hevm", L"image/heif-sequence", L"HEIF Image Sequence L-HEVC", types::hevm},
      {8, "hevs", L"image/heif-sequence", L"HEIF Image Sequence L-HEVC", types::hevs},
      {8, "avcs", L"image/heif-sequence", L"HEIF Image Sequence AVC", types::avcs},
      // https://aomediacodec.github.io/av1-avif/
      {8, "avif", L"image/avif", L"AVIF Image", types::avif},
      {8, "avis", L"image/avif", L"AVIF Image Sequence", types::avis},
  };
  if (!hr.ZeroExists()) {
    return None;
  }
  for (const auto &i : images) {
    if (mv.size() > i.offset && mv.IndexsWith(i.offset, i.magic)) {
      hr.assign(i.t, i.desc);
      hr.append(L"MIME", i.mime);
      return Found;
    }
  }
  return None;
}

// struct psd_header_t {
//   uint8_t sig[4];
//   uint16_t ver;
//   uint8_t reserved[6];
//   uint16_t alpha;
//   uint32_t height;
//   uint32_t width;
//   uint16_t depth;
//   uint16_t cm;
// };
// https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_19840

status_t LookupImages(bela::MemView mv, hazel_result &hr) {
  constexpr const uint8_t icoMagic[] = {0x00, 0x00, 0x01, 0x00};
  constexpr const uint8_t jpegMagic[] = {0xFF, 0xD8, 0xFF};
  constexpr const uint8_t jpeg2000Magic[] = {0x0, 0x0, 0x0, 0xC, 0x6A, 0x50, 0x20, 0xD, 0xA, 0x87, 0xA, 0x0};
  constexpr const uint8_t pngMagic[] = {0x89, 0x50, 0x4E, 0x47};
  constexpr const uint8_t gifMagic[] = {0x47, 0x49, 0x46};
  constexpr const uint8_t webpMagic[] = {0x57, 0x45, 0x42, 0x50};
  constexpr const uint8_t psdMagic[] = {0x38, 0x42, 0x50, 0x53};
  constexpr const size_t psdhlen = 4 + 2 + 6 + 2 + 4 + 4 + 2 + 2;
  switch (mv[0]) {
  case 0x0:
    if (mv.StartsWith(icoMagic)) {
      hr.assign(types::ico, L"ICO file format (.ico)");
      return Found;
    }
    if (mv.StartsWith(jpeg2000Magic)) {
      hr.assign(types::jp2, L"JPEG 2000 Image");
      return Found;
    }
    break;
  case 0x38:
    if (mv.StartsWith(psdMagic) && mv.size() > psdhlen) {
      // Version: always equal to 1.
      auto ver = bela::cast_frombe<uint16_t>((void *)(mv.data() + 4));
      if (ver == 1) {
        hr.assign(types::psd, L"Photoshop document file extension");
        return Found;
      }
    }
    break;
  case 0x42:
    if (mv.size() > 2 && mv[1] == 0x4D) {
      hr.assign(types::bmp, L"Bitmap image file format (.bmp)");
      return Found;
    }
    break;
  case 0x47:
    if (mv.StartsWith(gifMagic)) {
      constexpr size_t gmlen = sizeof(gifMagic);
      if (mv.size() > gmlen + 3 && mv[gmlen] == '8' && (mv[gmlen + 1] == '7' || mv[gmlen + 1] == '9') &&
          mv[gmlen + 2] == 'a') {
        hr.assign(types::gif, L"Graphics Interchange Format (.gif)");
        return Found;
      }
    }
    break;
  case 0x49:
    if (mv.size() > 9 && mv[1] == 0x49 && mv[2] == 0x2A && mv[3] == 0x0 && mv[8] == 0x43 && mv[9] == 0x52) {
      hr.assign(types::cr2, L"Canon 5D Mark IV CR2");
      return Found;
    }
    if (mv.size() > 3 && mv[1] == 0x49 && mv[2] == 0x2A && mv[3] == 0x0) {
      hr.assign(types::tif, L"Tagged Image File Format (.tif)");
      return Found;
    }
    if (mv.size() > 2 && mv[1] == 0x49 && mv[2] == 0xBC) {
      hr.assign(types::jxr, L"JPEG extended range");
      return Found;
    }
    break;
  case 0x4D:
    if (mv.size() > 9 && mv[0] == 0x4D && mv[1] == 0x4D && mv[2] == 0x0 && mv[3] == 0x2A && mv[8] == 0x43 &&
        mv[9] == 0x52) {
      hr.assign(types::cr2, L"Canon 5D Mark IV CR2");
      return Found;
    }
    if (mv.size() > 3 && mv[1] == 0x4D && mv[2] == 0x0 && mv[3] == 0x2A) {
      hr.assign(types::tif, L"Tagged Image File Format (.tif)");
      return Found;
    }
    break;
  case 0x57:
    if (mv.StartsWith(webpMagic)) {
      hr.assign(types::webp, L"WebP Image");
      return Found;
    }
    break;
  case 0x89:
    if (mv.StartsWith(pngMagic)) {
      hr.assign(types::png, L"Portable Network Graphics (.png)");
      return Found;
    }
    break;
  case 0xFF:
    if (mv.StartsWith(jpegMagic)) {
      hr.assign(types::jpg, L"JPEG Image");
      return Found;
    }
    break;
  default:
    break;
  }
  return LookupNewImages(mv, hr);
}

} // namespace hazel::internal
