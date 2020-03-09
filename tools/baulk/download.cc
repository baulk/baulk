/// baulk download utils
//
#include "baulk.hpp"
#include "process.hpp"
#include <bela/path.hpp>

namespace baulk::net {

bool CURLGet(std::wstring_view url, std::wstring_view dest,
             bela::error_code ec) {
  std::wstring curlexe;
  if (!bela::ExecutableExistsInPath(L"curl.exe", curlexe)) {
    return false;
  }
  Process p;
  auto exitcode = p.Execute(curlexe, L"-A=Wget/5.0 (Baulk)", L"--progress-bar",
                            L"-fS", L"--connect-timeout", L"15", L"--retry",
                            L"3", L"-o", dest, L"-L", url);
  if (exitcode != 0) {
    ec = bela::make_error_code(-1, L"curl exit code: ", exitcode);
    return false;
  }
  return true;
}

bool WebGet(std::wstring_view url, std::wstring_view dest,
            bela::error_code ec) {
  std::wstring wgetexe;
  if (!bela::ExecutableExistsInPath(L"wget.exe", wgetexe)) {
    return false;
  }
  Process p;
  auto exitcode = p.Execute(wgetexe, url, L"-O", dest);
  if (exitcode != 0) {
    ec = bela::make_error_code(-1, L"wget exit code: ", exitcode);
    return false;
  }
  return true;
}

} // namespace baulk::net
