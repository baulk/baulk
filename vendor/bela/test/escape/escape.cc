///
/// unicode escape L"CH\u2082O\u2083" => L"CH₂O"
#include <bela/escaping.hpp>
#include <bela/terminal.hpp>

int wmain() {
  bela::FPrintF(stderr, L"Humans cannot do without O\u2082\n");
  const wchar_t *msg = L"H\\u2082O \\U0001F496 \\xA9 \\x1b[32mcolour "
                       L"escape\\x1b[0m \\u2080\\u2081\\u2082 (2-9)";
  const auto ws =
      LR"("C:\Program Files\PowerShell\7-preview\pwsh.exe" -NoExit -Command "$Host.UI.RawUI.WindowTitle=\"Windows Pwsh 💙 (7 Preview)\"")";

  // H₂O 💖 © colour escape
  std::wstring dest;
  if (bela::CUnescape(msg, &dest)) {
    bela::FPrintF(stderr, L"\U0001F496: %s\n%s\n", msg, dest);
  }
  auto result = bela::CEscape(ws);
  bela::FPrintF(stderr, L"Escape:\n%s\n", result);
  return 0;
}
