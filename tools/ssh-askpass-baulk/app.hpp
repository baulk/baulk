///
#ifndef SSH_ASKPASS_APP
#define SSH_ASKPASS_APP
#include <bela/base.hpp>

namespace baulk {
class App {
public:
  App(std::wstring_view prompt_, std::wstring_view title_, bool echomode_)
      : prompt(prompt_), title(title_), echomode(echomode_) {
    // backgroud 235 237 239
    // button 183 191 196
    bkColor = RGB(235, 237, 239);
    btColor = RGB(183, 191, 196);
  }
  App(const App &) = delete;
  App &operator=(const App &) = delete;
  ~App();
  static App *GetThisFromHandle(HWND const window) noexcept {
    return reinterpret_cast<App *>(GetWindowLongPtrW(window, GWLP_USERDATA));
  }
  int run(HINSTANCE hInstance);
  static INT_PTR WINAPI WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                   LPARAM lParam);
  INT_PTR MessageHandler(UINT message, WPARAM wParam, LPARAM lParam);

private:
  bool Initialize(HWND window);
  bool calculateWindow(int &x, int &y, std::wstring &prompttext);
  std::wstring prompt;
  std::wstring title;
  HINSTANCE hInst{nullptr};
  HWND hWnd{nullptr};
  HWND hPrompt{nullptr};
  HWND hInput{nullptr};
  HWND hCancel{nullptr};
  HWND hOK{nullptr};
  HBRUSH hbrBkgnd{nullptr};
  HBRUSH hbrButton{nullptr};
  COLORREF bkColor{0};
  COLORREF btColor{0};
  bool echomode{false};
};
} // namespace baulk

#endif