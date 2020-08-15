///
#include <bela/codecvt.hpp> // test prompt length
#include <bela/str_split.hpp>
#include <bela/parseargv.hpp>
#include <bela/picker.hpp>
#include <bela/terminal.hpp>
#include <baulkrev.hpp>
#include "app.hpp"
#include "resource.h" // define id's

namespace baulk {
App::~App() {
  if (hbrBkgnd != nullptr) {
    DeleteObject(hbrBkgnd);
  }
}
int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_ASKPASS_WINDOW), NULL,
                              App::WindowProc, reinterpret_cast<LPARAM>(this));
}
// Typically, the dialog box procedure should return TRUE if it processed the
// message, and FALSE if it did not. If the dialog box procedure returns FALSE,
// the dialog manager performs the default dialog operation in response to the
// message.
INT_PTR WINAPI App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  App *app{nullptr};
  if (message == WM_INITDIALOG) {
    auto app = reinterpret_cast<App *>(lParam);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    app->Initialize(hWnd);
    return TRUE;
  }
  if ((app = GetThisFromHandle(hWnd)) != nullptr) {
    return app->MessageHandler(message, wParam, lParam);
  }
  return FALSE;
}

bool App::calculateWindow(int &x, int &y, std::wstring &prompttext) {
  auto hdc = GetDC(hPrompt); // test font max length
  std::vector<std::wstring_view> strv = bela::StrSplit(prompt, bela::ByChar('\n'));
  SIZE cursize{0};
  for (const auto &sv : strv) {
    std::wstring s(sv);
    if (!s.empty() && s.back() == '\r') {
      s.pop_back();
    }
    if (!prompttext.empty()) {
      prompttext.append(L"\r\n");
    }
    prompttext.append(s);
    GetTextExtentPoint32W(hdc, s.data(), static_cast<int>(s.size()), &cursize);
    x = (std::max)(x, static_cast<int>(cursize.cx));
    y += cursize.cy + 5;
  }
  ReleaseDC(hWnd, hdc);
  return true;
}

// backgroud 235 237 239
// button 183 191 196
bool App::Initialize(HWND window) {
  hWnd = window;
  int maxlen = 0;
  int pheight = 0;
  std::wstring prompttext;
  auto hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_ASKPASS_ABOUT,
              L"About Askpass Utility for Baulk");
  hPrompt = GetDlgItem(hWnd, IDS_PROMPT_CONTENT);
  hInput = GetDlgItem(hWnd, IDE_ASKPASS_INPUT);
  constexpr auto es = WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | ES_LEFT;
  constexpr auto exs = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY;
  SetWindowLongPtrW(hInput, GWL_EXSTYLE, exs);
  if (!echomode) {
    SetWindowLongPtrW(hInput, GWL_STYLE, es | ES_PASSWORD);
    SendDlgItemMessage(hWnd, IDE_ASKPASS_INPUT, EM_SETPASSWORDCHAR, (WPARAM)L'•', (LPARAM)0);
  } else {
    SetWindowLongPtrW(hInput, GWL_STYLE, es);
  }
  hCancel = GetDlgItem(hWnd, IDB_ASKPASS_CANCEL);
  hOK = GetDlgItem(hWnd, IDB_ASKPASS_OK);
  // hPrompt must initialized
  calculateWindow(maxlen, pheight, prompttext);
  maxlen = (std::max)(maxlen, 300);
  auto height = 40 + 10 + pheight + 35 + 35 + 35; // line count
  auto width = (std::max)(maxlen + 40, 300);
  SetWindowPos(hWnd, nullptr, 0, 0, width, height,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED);
  HDWP hdwp = BeginDeferWindowPos(4); //
  DeferWindowPos(hdwp, hPrompt, nullptr, 20, 10, maxlen, pheight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  // changel hInput layout
  DeferWindowPos(hdwp, hInput, nullptr, 20, 10 + pheight + 10, width - 40 - 20, 30,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  DeferWindowPos(hdwp, hCancel, nullptr, width - 137 * 2 - 6 - 40, 10 + pheight + 50, 137, 32,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  DeferWindowPos(hdwp, hOK, nullptr, width - 137 - 40, 10 + pheight + 50, 137, 32,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  EndDeferWindowPos(hdwp);
  HICON icon = LoadIconW(hInst, MAKEINTRESOURCEW(ICON_MAIN));
  SetWindowTextW(hWnd, title.data());
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
  SetWindowTextW(hPrompt, prompttext.data());
  // Set the default push button to "OK."
  // SendMessage(hWnd, DM_SETDEFID, (WPARAM)IDB_ASKPASS_OK, (LPARAM)0);
  return true;
}

inline std::wstring windowcontent(HWND hWnd) {
  auto l = GetWindowTextLengthW(hWnd);
  if (l == 0 || l > 64 * 1024) {
    return L"";
  }
  std::wstring s(l + 1, L'\0');
  GetWindowTextW(hWnd, &s[0], l + 1); //// Null T
  s.resize(l);
  return s;
}

// Handler
INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CTLCOLORDLG:
    if (hbrBkgnd == nullptr) {
      hbrBkgnd = CreateSolidBrush(bkColor);
    }
    return (INT_PTR)hbrBkgnd;
  case WM_CTLCOLORSTATIC: {
    HDC hdc = (HDC)wParam;
    if (reinterpret_cast<HWND>(lParam) == hInput) {
      return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }
    SetBkColor(hdc, bkColor);
    SetBkMode(hdc, TRANSPARENT);
    if (hbrBkgnd == nullptr) {
      hbrBkgnd = CreateSolidBrush(bkColor);
    }
    return (INT_PTR)hbrBkgnd;
  }
  case WM_CTLCOLORBTN: {
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, btColor);
    SetBkMode(hdc, TRANSPARENT);
    if (hbrButton == nullptr) {
      hbrButton = CreateSolidBrush(btColor);
    }
    return (INT_PTR)hbrButton;
  } break;
  case WM_SYSCOMMAND:
    if (LOWORD(wParam) == IDM_ASKPASS_ABOUT) {
      bela::BelaMessageBox(hWnd, L"Askpass Utility for Baulk", BAULK_APPVERSION, BAULK_APPLINK,
                           bela::mbs_t::ABOUT);
      return TRUE;
    }
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDB_ASKPASS_OK:
      bela::FPrintF(stdout, L"%s\n", windowcontent(hInput));
      DestroyWindow(hWnd);
      return TRUE;
    case IDB_ASKPASS_CANCEL: /// lookup command
      DestroyWindow(hWnd);
      return TRUE;
    default:
      return FALSE;
    }
    break;
  case WM_CLOSE:
    DestroyWindow(hWnd);
    return TRUE;
  case WM_DESTROY:
    PostQuitMessage(0);
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

} // namespace baulk