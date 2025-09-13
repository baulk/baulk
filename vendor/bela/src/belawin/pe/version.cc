//
#include <bela/pe.hpp>
#include <bela/buffer.hpp>

// https://github.com/chromium/chromium/blob/master/base/file_version_info_win.cc

namespace bela::pe {

struct LanguageAndCodePage {
  WORD language;
  WORD codePage;
};

// Returns the \VarFileInfo\Translation value extracted from the
// VS_VERSION_INFO resource in |data|.
LanguageAndCodePage *GetTranslate(const void *data) {
  static constexpr wchar_t kTranslation[] = L"\\VarFileInfo\\Translation";
  LPVOID translate = nullptr;
  UINT dummy_size;
  if (::VerQueryValue(data, kTranslation, &translate, &dummy_size) != 0) {
    return static_cast<LanguageAndCodePage *>(translate);
  }
  return nullptr;
}

const VS_FIXEDFILEINFO *GetVsFixedFileInfo(const void *data) {
  static constexpr wchar_t kRoot[] = L"\\";
  LPVOID fixed_file_info = nullptr;
  UINT dummy_size;
  if (::VerQueryValue(data, kRoot, &fixed_file_info, &dummy_size) != 0) {
    //
    return nullptr;
  }
  return static_cast<VS_FIXEDFILEINFO *>(fixed_file_info);
}

struct VersionLoader {
  bela::Buffer buffer;
  WORD language;
  WORD codePage;
  LANGID defaultLangage;
  bool initialize(std::wstring_view file, bela::error_code &ec);
  bool getValue(const wchar_t *name, std::wstring &value) const;
};

bool VersionLoader::initialize(std::wstring_view file, bela::error_code &ec) {
  DWORD dwhandle{0};
  auto N = GetFileVersionInfoSizeW(file.data(), &dwhandle);
  if (N == 0) {
    ec = bela::make_system_error_code(L"GetFileVersionInfoSizeW ");
    return false;
  }
  buffer.grow(N);
  if (GetFileVersionInfoW(file.data(), 0, N, buffer.data()) != TRUE) {
    ec = bela::make_system_error_code(L"GetFileVersionInfoW");
    return false;
  }
  buffer.size() = static_cast<size_t>(N);
  auto translate = GetTranslate(buffer.data());
  if (translate == nullptr) {
    ec = bela::make_error_code(L"no found translate");
    return false;
  }
  language = translate->language;
  codePage = translate->codePage;
  defaultLangage = GetUserDefaultLangID();
  return true;
}

std::wstring_view cleanupString(void *valuePtr, size_t n) {
  std::wstring_view sv{reinterpret_cast<const wchar_t *>(valuePtr), n};
  if (auto pos = sv.find(L'\0'); pos != std::wstring_view::npos) {
    return sv.substr(0, pos);
  }
  return sv;
}

bool VersionLoader::getValue(const wchar_t *name, std::wstring &value) const {
  const struct LanguageAndCodePage langCodepages[] = {
      // Use the language and codepage from the DLL.
      {.language = language, .codePage = codePage},
      // Use the default language and codepage from the DLL.
      {.language = defaultLangage, .codePage = codePage},
      // Use the language from the DLL and Latin codepage (most common).
      {.language = language, .codePage = 1252},
      // Use the default language and Latin codepage (most common).
      {.language = defaultLangage, .codePage = 1252},
  };
  for (const auto &lc : langCodepages) {
    auto block = bela::StringCat(L"\\StringFileInfo\\", bela::Hex(lc.language, bela::kZeroPad4),
                                 bela::Hex(lc.codePage, bela::kZeroPad4), L"\\", name);
    LPVOID valuePtr = nullptr;
    uint32_t size;
    if (VerQueryValueW(buffer.data(), block.data(), (&valuePtr), &size) == TRUE && valuePtr != nullptr && size > 0) {
      value.assign(cleanupString(valuePtr, size - 1));
    }
  }
  return false;
}

std::optional<Version> Lookup(std::wstring_view file, bela::error_code &ec) {
  VersionLoader vl;
  if (!vl.initialize(file, ec)) {
    return std::nullopt;
  }
  Version vi;
  vl.getValue(L"CompanyName", vi.CompanyName);
  vl.getValue(L"FileDescription", vi.FileDescription);
  vl.getValue(L"FileVersion", vi.FileVersion);
  vl.getValue(L"InternalName", vi.InternalName);
  vl.getValue(L"LegalCopyright", vi.LegalCopyright);
  vl.getValue(L"OriginalFileName", vi.OriginalFileName);
  vl.getValue(L"ProductName", vi.ProductName);
  vl.getValue(L"ProductVersion", vi.ProductVersion);
  vl.getValue(L"Comments", vi.Comments);
  vl.getValue(L"LegalTrademarks", vi.LegalTrademarks);
  vl.getValue(L"PrivateBuild", vi.PrivateBuild);
  vl.getValue(L"SpecialBuild", vi.SpecialBuild);
  return std::make_optional(std::move(vi));
}
} // namespace bela::pe