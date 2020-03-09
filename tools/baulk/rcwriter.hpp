#ifndef BAULK_RCWRITER_HPP
#define BAULK_RCWRITER_HPP
#include <string>
#include <string_view>
#include <bela/strcat.hpp>
#include <bela/str_split.hpp>

namespace baulk::rc {

struct VersionPart {
  int MajorPart{0};
  int MinorPart{0};
  int BuildPart{0};
  int PrivatePart{0};
};

inline VersionPart MakeVersionPart(std::wstring_view vs) {
  VersionPart vp;
  std::vector<std::wstring_view> vsv =
      bela::StrSplit(vs, bela::ByChar('.'), bela::SkipEmpty());
  if (!vsv.empty()) {
    bela::SimpleAtoi(vsv[0], &vp.MajorPart);
  }
  if (vsv.size() > 1) {
    bela::SimpleAtoi(vsv[1], &vp.MinorPart);
  }
  if (vsv.size() > 2) {
    bela::SimpleAtoi(vsv[2], &vp.BuildPart);
  }
  if (vsv.size() > 3) {
    bela::SimpleAtoi(vsv[3], &vp.PrivatePart);
  }
  return vp;
}

class Writer {
public:
  Writer() { buffer.reserve(4096); }
  Writer(const Writer &) = delete;
  Writer &operator=(const Writer &) = delete;
  Writer &Prefix() {
    buffer.assign(L"//Baulk generated resource script.\n#include "
                  L"\"windows.h\"\n\nVS_VERSION_INFO VERSIONINFO\n");
    return *this;
  }
  Writer &FileVersion(int MajorPart, int MinorPart, int BuildPart,
                      int PrivatePart) {
    bela::StrAppend(&buffer, L"FILEVERSION ", MajorPart, L", ", MinorPart,
                    L", ", BuildPart, L", ", PrivatePart);
    return *this;
  }
  Writer &ProductVersion(int MajorPart, int MinorPart, int BuildPart,
                         int PrivatePart) {
    bela::StrAppend(&buffer, L"FILEVERSION ", MajorPart, L", ", MinorPart,
                    L", ", BuildPart, L", ", PrivatePart);
    return *this;
  }
  Writer &PreVersion() {
    buffer.append(
        L"FILEFLAGSMASK 0x3fL\nFILEFLAGS 0x0L\nFILEOS 0x40004L\nFILETYPE "
        L"0x1L\nFILESUBTYPE 0x0L\nBEGIN\nBLOCK "
        L"\"StringFileInfo\"\nBEGIN\nBLOCK \"000904b0\"\nBEGIN\n");
    return *this;
  }
  Writer &Version(std::wstring_view name, std::wstring_view value) {
    bela::StrAppend(&buffer, L"VALUE \"", name, L"\" L\"", value, L"\"\n");
    return *this;
  }
  Writer &AfterVersion() {
    buffer.append(L"END\nEND\nBLOCK \"VarFileInfo\"\nBEGIN\nVALUE "
                  L"\"Translation\", 0x9, 1200\nEND\nEND\n\n");
    return *this;
  }
  bool FlushFile(std::wstring_view file) const {
    (void)file;
    return true;
  }

private:
  std::wstring buffer;
};
} // namespace baulk::rc

#endif
