#ifndef BAULK_RCWRITER_HPP
#define BAULK_RCWRITER_HPP
#include <string>
#include <string_view>
#include <bela/str_cat.hpp>
#include <bela/str_split.hpp>
#include <bela/str_replace.hpp>
#include <bela/pe.hpp>
#include <bela/io.hpp>

namespace baulk::rc {

struct VersionPart {
  int MajorPart{0};
  int MinorPart{0};
  int BuildPart{0};
  int PrivatePart{0};
};

inline VersionPart MakeVersionPart(std::wstring_view vs) {
  VersionPart vp;
  std::vector<std::wstring_view> vsv = bela::StrSplit(vs, bela::ByChar('.'), bela::SkipEmpty());
  if (!vsv.empty()) {
    (void)bela::SimpleAtoi(vsv[0], &vp.MajorPart);
  }
  if (vsv.size() > 1) {
    (void)bela::SimpleAtoi(vsv[1], &vp.MinorPart);
  }
  if (vsv.size() > 2) {
    (void)bela::SimpleAtoi(vsv[2], &vp.BuildPart);
  }
  if (vsv.size() > 3) {
    (void)bela::SimpleAtoi(vsv[3], &vp.PrivatePart);
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
  Writer &FileVersion(int MajorPart, int MinorPart, int BuildPart, int PrivatePart) {
    bela::StrAppend(&buffer, L"FILEVERSION ", MajorPart, L", ", MinorPart, L", ", BuildPart, L", ", PrivatePart, L"\n");
    return *this;
  }
  Writer &ProductVersion(int MajorPart, int MinorPart, int BuildPart, int PrivatePart) {
    bela::StrAppend(&buffer, L"PRODUCTVERSION ", MajorPart, L", ", MinorPart, L", ", BuildPart, L", ", PrivatePart,
                    L"\n");
    return *this;
  }
  Writer &PreVersion() {
    buffer.append(L"FILEFLAGSMASK 0x3fL\nFILEFLAGS 0x0L\nFILEOS 0x40004L\nFILETYPE "
                  L"0x1L\nFILESUBTYPE 0x0L\nBEGIN\nBLOCK "
                  L"\"StringFileInfo\"\nBEGIN\nBLOCK \"000904b0\"\nBEGIN\n");
    return *this;
  }
  Writer &Version(std::wstring_view name, std::wstring_view value) {
    bela::StrAppend(&buffer, L"VALUE \"", name, L"\", L\"", value, L"\"\n");
    return *this;
  }
  Writer &VersionEx(std::wstring_view name, std::wstring_view value, std::wstring_view dv) {
    return Version(name, value.empty() ? dv : value);
  }
  Writer &AfterVersion() {
    buffer.append(L"END\nEND\nBLOCK \"VarFileInfo\"\nBEGIN\nVALUE "
                  L"\"Translation\", 0x9, 1200\nEND\nEND\n\n");
    return *this;
  }
  bool WriteVersion(bela::pe::Version &vi, std::wstring_view file, bela::error_code &ec) {
    Prefix();
    auto fvp = MakeVersionPart(vi.FileVersion);
    FileVersion(fvp.MajorPart, fvp.MinorPart, fvp.BuildPart, fvp.PrivatePart);
    auto pvp = MakeVersionPart(vi.ProductVersion);
    ProductVersion(pvp.MajorPart, pvp.MinorPart, pvp.BuildPart, pvp.PrivatePart);
    PreVersion();
    std::wstring legalCopyright;
    if (vi.LegalCopyright.empty()) {
      legalCopyright = L"No checked copyright";
    } else {
      legalCopyright = bela::StrReplaceAll(vi.LegalCopyright, {{L"(c)", L"\xA9"}, {L"(C)", L"\xA9"}});
    }
    Version(L"CompanyName", vi.CompanyName);
    Version(L"FileDescription", vi.FileDescription);
    Version(L"InternalName", vi.InternalName);
    Version(L"LegalCopyright", legalCopyright);
    Version(L"OriginalFileName", vi.OriginalFileName);
    Version(L"ProductName", vi.ProductName);
    Version(L"ProductVersion", vi.ProductVersion);
    AfterVersion();
    return bela::io::WriteText(buffer, file, ec);
  }

private:
  std::wstring buffer;
};
} // namespace baulk::rc

#endif
