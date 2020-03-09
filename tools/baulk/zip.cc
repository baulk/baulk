///
#include <bela/base.hpp>
#include <bela/path.hpp>
#include "process.hpp"
#include "fs.hpp"

namespace baulk::zip {
inline std::optional<std::wstring> LookupPwsh(bela::error_code &ec) {
  std::wstring pwsh;
  if (bela::ExecutableExistsInPath(L"pwsh.exe", pwsh)) {
    return std::make_optional(std::move(pwsh));
  }
  // Powershell
  if (bela::ExecutableExistsInPath(L"powershell.exe", pwsh)) {
    return std::make_optional(std::move(pwsh));
  }
  return std::nullopt;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir,
                bela::error_code &ec) {
  auto pwsh = LookupPwsh(ec);
  if (!pwsh) {
    return false;
  }
  auto command = bela::StringCat(L"Expand-Archive -Path \"", src,
                                 L"\" -DestinationPath \"", outdir, L"\"");
  baulk::Process process;
  if (process.Execute(*pwsh, L"-Command", command) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}

} // namespace baulk::zip