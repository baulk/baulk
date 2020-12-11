//////// video audio todo
#include "hazelinc.hpp"

namespace hazel::internal {
inline bool IsMp3(const uint8_t *buf, size_t size) {
  return (size > 2 && ((buf[0] == 0x49 && buf[1] == 0x44 && buf[2] == 0x33) || (buf[0] == 0xFF && buf[1] == 0xfb)));
}

inline bool IsM4a(const uint8_t *buf, size_t size) {
  return (size > 10 && ((buf[4] == 0x66 && buf[5] == 0x74 && buf[6] == 0x79 && buf[7] == 0x70 && buf[8] == 0x4D &&
                         buf[9] == 0x34 && buf[10] == 0x41) ||
                        (buf[0] == 0x4D && buf[1] == 0x34 && buf[2] == 0x41 && buf[3] == 0x20)));
}

inline bool IsAac(const uint8_t *buf, size_t size) {
  return (size > 1 && buf[0] == 0xFF && (buf[1] == 0xF1 || buf[1] == 0xF9));
}

inline bool IsMov(const uint8_t *buf, size_t size) {
  return (size > 15 && ((buf[0] == 0x0 && buf[1] == 0x0 && buf[2] == 0x0 && buf[3] == 0x14 && buf[4] == 0x66 &&
                         buf[5] == 0x74 && buf[6] == 0x79 && buf[7] == 0x70) ||
                        (buf[4] == 0x6d && buf[5] == 0x6f && buf[6] == 0x6f && buf[7] == 0x76) ||
                        (buf[4] == 0x6d && buf[5] == 0x64 && buf[6] == 0x61 && buf[7] == 0x74) ||
                        (buf[12] == 0x6d && buf[13] == 0x64 && buf[14] == 0x61 && buf[15] == 0x74)));
}

inline bool Mpeg(const uint8_t *buf, size_t size) {
  return (size > 3 && buf[0] == 0x0 && buf[1] == 0x0 && buf[2] == 0x1 && buf[3] >= 0xb0 && buf[3] <= 0xbf);
}

status_t lookup_mediaaudio(bela::MemView mv, FileAttributeTable &fat) {
  constexpr const uint8_t midiMagic[] = {0x4D, 0x54, 0x68, 0x64};
  constexpr const uint8_t oggMagic[] = {0x4F, 0x67, 0x67, 0x53};
  constexpr const uint8_t flacMagic[] = {0x66, 0x4C, 0x61, 0x43};
  constexpr const uint8_t wavMagic[] = {0x52, 0x49, 0x46, 0x46, 0x57, 0x41, 0x56, 0x45};
  constexpr const uint8_t amrMagic[] = {0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A};
  if (mv.StartsWith(midiMagic)) {
    fat.assign(L"MIDI Audio", types::midi);
    return Found;
  }
  if (IsMp3(mv.data(), mv.size())) {
    fat.assign(L"MP3 Audio", types::mp3);
    return Found;
  }
  if (IsM4a(mv.data(), mv.size())) {
    fat.assign(L"M4A Audio", types::m4a);
    return Found;
  }
  if (mv.StartsWith(oggMagic)) {
    fat.assign(L"OGG Audio/Video", types::ogg);
    return Found;
  }
  if (mv.StartsWith(flacMagic)) {
    fat.assign(L"Free Lossless Audio Codec", types::flac);
    return Found;
  }
  if (mv.StartsWith(wavMagic)) {
    fat.assign(L"Waveform Audio File Format", types::wav);
    return Found;
  }
  if (mv.StartsWith(amrMagic)) {
    fat.assign(L"Adaptive Multi-Rate audio codecat", types::amr);
    return Found;
  }
  if (IsAac(mv.data(), mv.size())) {
    fat.assign(L"Advanced Audio Coding", types::aac);
    return Found;
  }
  return None;
}

inline bool IsM4v(const uint8_t *buf, size_t size) {
  return (size > 10 && buf[4] == 0x66 && buf[5] == 0x74 && buf[6] == 0x79 && buf[7] == 0x70 && buf[8] == 0x4D &&
          buf[9] == 0x34 && buf[10] == 0x56);
}

inline bool IsMkv(const uint8_t *buf, size_t size) {
  return (size > 15 && buf[0] == 0x1A && buf[1] == 0x45 && buf[2] == 0xDF && buf[3] == 0xA3 && buf[4] == 0x93 &&
          buf[5] == 0x42 && buf[6] == 0x82 && buf[7] == 0x88 && buf[8] == 0x6D && buf[9] == 0x61 && buf[10] == 0x74 &&
          buf[11] == 0x72 && buf[12] == 0x6F && buf[13] == 0x73 && buf[14] == 0x6B && buf[15] == 0x61) ||
         (size > 38 && buf[31] == 0x6D && buf[32] == 0x61 && buf[33] == 0x74 && buf[34] == 0x72 && buf[35] == 0x6f &&
          buf[36] == 0x73 && buf[37] == 0x6B && buf[38] == 0x61);
}

inline bool IsAvi(const uint8_t *buf, size_t size) {
  return (size > 10 && buf[0] == 0x52 && buf[1] == 0x49 && buf[2] == 0x46 && buf[3] == 0x46 && buf[8] == 0x41 &&
          buf[9] == 0x56 && buf[10] == 0x49);
}
inline bool IsMpeg(const uint8_t *buf, size_t size) {
  return (size > 3 && buf[0] == 0x0 && buf[1] == 0x0 && buf[2] == 0x1 && buf[3] >= 0xb0 && buf[3] <= 0xbf);
}

inline bool IsMp4(const uint8_t *buf, size_t size) {
  return (size > 11 && (buf[4] == 'f' && buf[5] == 't' && buf[6] == 'y' && buf[7] == 'p') &&
          ((buf[8] == 'a' && buf[9] == 'v' && buf[10] == 'c' && buf[11] == '1') ||
           (buf[8] == 'd' && buf[9] == 'a' && buf[10] == 's' && buf[11] == 'h') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == '2') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == '3') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == '4') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == '5') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == '6') ||
           (buf[8] == 'i' && buf[9] == 's' && buf[10] == 'o' && buf[11] == 'm') ||
           (buf[8] == 'm' && buf[9] == 'm' && buf[10] == 'p' && buf[11] == '4') ||
           (buf[8] == 'm' && buf[9] == 'p' && buf[10] == '4' && buf[11] == '1') ||
           (buf[8] == 'm' && buf[9] == 'p' && buf[10] == '4' && buf[11] == '2') ||
           (buf[8] == 'm' && buf[9] == 'p' && buf[10] == '4' && buf[11] == 'v') ||
           (buf[8] == 'm' && buf[9] == 'p' && buf[10] == '7' && buf[11] == '1') ||
           (buf[8] == 'M' && buf[9] == 'S' && buf[10] == 'N' && buf[11] == 'V') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'A' && buf[11] == 'S') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'S' && buf[11] == 'C') ||
           (buf[8] == 'N' && buf[9] == 'S' && buf[10] == 'D' && buf[11] == 'C') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'S' && buf[11] == 'H') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'S' && buf[11] == 'M') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'S' && buf[11] == 'P') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'S' && buf[11] == 'S') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'X' && buf[11] == 'C') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'X' && buf[11] == 'H') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'X' && buf[11] == 'M') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'X' && buf[11] == 'P') ||
           (buf[8] == 'N' && buf[9] == 'D' && buf[10] == 'X' && buf[11] == 'S') ||
           (buf[8] == 'F' && buf[9] == '4' && buf[10] == 'V' && buf[11] == ' ') ||
           (buf[8] == 'F' && buf[9] == '4' && buf[10] == 'P' && buf[11] == ' ')));
}

status_t lookup_mediavideo(bela::MemView mv, FileAttributeTable &fat) {
  constexpr const uint8_t webmMagic[] = {0x1A, 0x45, 0xDF, 0xA3};
  constexpr const uint8_t wmvMagic[] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD6};
  constexpr const uint8_t flvMagic[] = {0x46, 0x4C, 0x56, 0x01};
  if (IsM4v(mv.data(), mv.size())) {
    fat.assign(L"M4V Video", types::m4v);
    return Found;
  }
  if (IsMkv(mv.data(), mv.size())) {
    fat.assign(L"Matroska Multimedia Container (.mkv)", types::mkv);
    return Found;
  }
  if (mv.StartsWith(webmMagic)) {
    fat.assign(L"WebM Video", types::webm);
    return Found;
  }
  if (IsAvi(mv.data(), mv.size())) {
    fat.assign(L"Audio Video Interleaved (.avi)", types::avi);
    return Found;
  }
  if (mv.StartsWith(wmvMagic)) {
    fat.assign(L"", types::wmv);
    return Found;
  }
  if (IsMpeg(mv.data(), mv.size())) {
    fat.assign(L"MPEG Video", types::mpeg);
    return Found;
  }
  if (mv.StartsWith(flvMagic)) {
    fat.assign(L"Flash Video", types::flv);
    return Found;
  }
  if (IsMp4(mv.data(), mv.size())) {
    fat.assign(L"MPEG-4 Part 14 Video (.mp4)", types::mp4);
    return Found;
  }
  return None;
}

status_t LookupMedia(bela::MemView mv, FileAttributeTable &fat) {
  if (lookup_mediaaudio(mv, fat) == Found) {
    return Found;
  }
  return lookup_mediavideo(mv, fat);
}
} // namespace hazel::internal
