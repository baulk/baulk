//
#include <bela/pe.hpp>

namespace bela::pe {

class Exposer {
public:
  Exposer() = default;
  Exposer(const Exposer &) = delete;
  Exposer &operator=(const Exposer &) = delete;
  ~Exposer() {
    if (buffer != nullptr) {
      HeapFree(GetProcessHeap(), 0, buffer);
    }
  }
  bool Initialize(std::wstring_view file, bela::error_code &ec) {
    DWORD dwhandle{0};
    auto N = GetFileVersionInfoSizeW(file.data(), &dwhandle);
    if (N == 0) {
      ec = bela::make_system_error_code();
      return false;
    }
    buffer = reinterpret_cast<uint8_t *>(HeapAlloc(GetProcessHeap(), 0, N));
    if (buffer == nullptr) {
      ec = bela::make_system_error_code();
      return false;
    }
    if (GetFileVersionInfoW(file.data(), 0, N, buffer) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }
  bool GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough /*= FALSE*/) {
    LPWORD lpwData;

    for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2) {
      if (*lpwData == wLangId) {
        dwId = *((DWORD *)lpwData);
        return true;
      }
    }

    if (!bPrimaryEnough) {
      return false;
    }

    for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2) {
      if (((*lpwData) & 0x00FF) == (wLangId & 0x00FF)) {
        dwId = *((DWORD *)lpwData);
        return true;
      }
    }

    return false;
  }
  void VerQuery(VersionInfo &vi) {
    LPVOID info{nullptr};
    UINT ilen{0};
    VerQueryValueW(buffer, L"\\VarFileInfo\\Translation", &info, &ilen);
    DWORD dwLangCode = 0;
    if (!GetTranslationId(info, ilen, GetUserDefaultLangID(), dwLangCode, FALSE)) {
      if (!GetTranslationId(info, ilen, GetUserDefaultLangID(), dwLangCode, TRUE)) {
        if (!GetTranslationId(info, ilen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE)) {
          if (!GetTranslationId(info, ilen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
            // use the first one we can get
            dwLangCode = *(reinterpret_cast<DWORD *>(info));
        }
      }
    }
    // dwLangCode&0x0000FFFF, (dwLangCode&0xFFFF0000)>>16

    auto block =
        bela::StringCat(L"\\StringFileInfo\\", bela::AlphaNum(bela::Hex(dwLangCode & 0x0000FFFF, bela::kZeroPad4)),
                        bela::AlphaNum(bela::Hex((dwLangCode & 0xFFFF0000) >> 16, bela::kZeroPad4)), L"\\");
    auto query = [&](std::wstring_view name, std::wstring &val) {
      auto k = bela::StringCat(block, name);
      if (VerQueryValueW(buffer, k.data(), &info, &ilen) == TRUE) {
        if (ilen > 0) {
          val.assign(reinterpret_cast<wchar_t *>(info), ilen - 1);
        }
      }
    };
    query(L"CompanyName", vi.CompanyName);
    query(L"FileDescription", vi.FileDescription);
    query(L"FileVersion", vi.FileVersion);
    query(L"InternalName", vi.InternalName);
    query(L"LegalCopyright", vi.LegalCopyright);
    query(L"OriginalFileName", vi.OriginalFileName);
    query(L"ProductName", vi.ProductName);
    query(L"ProductVersion", vi.ProductVersion);
    query(L"Comments", vi.Comments);
    query(L"LegalTrademarks", vi.LegalTrademarks);
    query(L"PrivateBuild", vi.PrivateBuild);
    query(L"SpecialBuild", vi.SpecialBuild);
  }

private:
  uint8_t *buffer{nullptr};
};

std::optional<VersionInfo> ExposeVersion(std::wstring_view file, bela::error_code &ec) {
  Exposer exposer;
  if (!exposer.Initialize(file, ec)) {
    return std::nullopt;
  }
  VersionInfo vi;
  exposer.VerQuery(vi);
  return std::make_optional(std::move(vi));
}
} // namespace bela::pe