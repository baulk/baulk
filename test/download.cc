/// baulk download utils
//
#include <bela/process.hpp>
#include <bela/simulator.hpp>

namespace baulk::net {

bool CURLGet(std::wstring_view url, std::wstring_view dest, bela::error_code &ec) {
  std::wstring curlexe;
  if (!bela::env::LookPath(L"curl.exe", curlexe, true)) {
    return false;
  }
  bela::process::Process p;
  auto exitcode = p.Execute(curlexe, L"-A=Wget/5.0 (Baulk)", L"--progress-bar", L"-fS", L"--connect-timeout", L"15",
                            L"--retry", L"3", L"-o", dest, L"-L", url);
  if (exitcode != 0) {
    ec = bela::make_error_code(-1, L"curl exit code: ", exitcode);
    return false;
  }
  return true;
}

bool WebGet(std::wstring_view url, std::wstring_view dest, bela::error_code &ec) {
  std::wstring wgetexe;
  if (!bela::env::LookPath(L"wget.exe", wgetexe, true)) {
    return false;
  }
  bela::process::Process p;
  auto exitcode = p.Execute(wgetexe, url, L"-O", dest);
  if (exitcode != 0) {
    ec = bela::make_error_code(-1, L"wget exit code: ", exitcode);
    return false;
  }
  return true;
}

} // namespace baulk::net
