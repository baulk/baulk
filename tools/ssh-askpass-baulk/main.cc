/// ssh-askpass-baulk
#include <Windows.h>
#include <wincred.h>
#include <string_view>
#include <bela/codecvt.hpp> // test prompt length
#include <bela/str_split.hpp>
#include <bela/parseargv.hpp>

// size_t PromptMaxLength(std::wstring_view prompt) {
//   std::vector<std::wstring_view> strv =
//       bela::StrSplit(prompt, bela::ByChar('\n'), bela::SkipEmpty());
//   size_t maxlen = 0;
//   for (auto s : strv) {
//     maxlen = (std::max)(maxlen, static_cast<size_t>(bela::StringWidth(s)));
//   }
//   return maxlen;
// }

int credentials(std::wstring_view prompt, std::wstring_view user) {
  //
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  //
  return 0;
}