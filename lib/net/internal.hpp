#ifndef BAULK_NET_NETINTERNAL_HPP
#define BAULK_NET_NETINTERNAL_HPP

namespace baulk::net {
struct UrlComponets {
  std::wstring host;
  std::wstring filename;
  std::wstring uri;
  int nPort{80};
  int nScheme{INTERNET_SCHEME_HTTPS};
  inline auto TlsFlag() const { return nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0; }
};

inline std::wstring UrlPathName(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  return std::wstring(pv.back());
}

inline bool CrackUrl(std::wstring_view url, UrlComponets &uc) {
  URL_COMPONENTSW urlcomp;
  ZeroMemory(&urlcomp, sizeof(urlcomp));
  urlcomp.dwStructSize = sizeof(urlcomp);
  urlcomp.dwSchemeLength = (DWORD)-1;
  urlcomp.dwHostNameLength = (DWORD)-1;
  urlcomp.dwUrlPathLength = (DWORD)-1;
  urlcomp.dwExtraInfoLength = (DWORD)-1;
  if (WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), 0, &urlcomp) != TRUE) {
    return false;
  }
  uc.host.assign(urlcomp.lpszHostName, urlcomp.dwHostNameLength);
  std::wstring_view urlpath{urlcomp.lpszUrlPath, urlcomp.dwUrlPathLength};
  uc.filename = UrlPathName(urlpath);
  uc.uri = bela::StringCat(urlpath, std::wstring_view(urlcomp.lpszExtraInfo, urlcomp.dwExtraInfoLength));
  uc.nPort = urlcomp.nPort;
  uc.nScheme = urlcomp.nScheme;
  return true;
}
} // namespace baulk::net

#endif