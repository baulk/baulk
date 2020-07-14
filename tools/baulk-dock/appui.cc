///
#include <bela/base.hpp>
#include <Windowsx.h>
#include <cassert>
#include <Prsht.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <PathCch.h>
#include <ShellScalingAPI.h>
#include <array>
#include <bela/picker.hpp>
#include <baulkversion.h>
#include "appui.hpp"

namespace baulk::dock {
// style
constexpr const auto noresizewnd =
    (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_MINIMIZEBOX);
constexpr const auto wexstyle =
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY;

constexpr const auto cbstyle =
    WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS;
constexpr const auto chboxstyle = BS_PUSHBUTTON | BS_TEXT | BS_DEFPUSHBUTTON | BS_CHECKBOX |
                                  BS_AUTOCHECKBOX | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE;
constexpr const auto pbstyle = BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE;

// Resources Safe Release
template <typename I> inline void Free(I **i) {
  if (*i != nullptr) {
    (*i)->Release();
  }
  *i = nullptr;
}

HRESULT LoadResourceBitmap(HINSTANCE hInst, ID2D1RenderTarget *renderTarget,
                           IWICImagingFactory *pIWICFactory, PCWSTR resourceName,
                           PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight,
                           ID2D1Bitmap **ppBitmap) {
  HRESULT hr = S_OK;
  IWICBitmapDecoder *pDecoder = nullptr;
  IWICBitmapFrameDecode *pSource = nullptr;
  IWICStream *pStream = nullptr;
  IWICFormatConverter *pConverter = nullptr;
  IWICBitmapScaler *pScaler = nullptr;
  HRSRC imageResHandle = nullptr;
  HGLOBAL imageResDataHandle = nullptr;
  void *pImageFile = nullptr;
  DWORD imageFileSize = 0;
  auto closer = bela::finally([&] {
    Free(&pDecoder);
    Free(&pSource);
    Free(&pStream);
    Free(&pConverter);
    Free(&pScaler);
  });
  // Find the resource then load it
  imageResHandle = FindResourceW(hInst, resourceName, resourceType);
  if (imageResHandle == nullptr) {
    return E_FAIL;
  }
  imageResDataHandle = LoadResource(hInst, imageResHandle);
  if (imageResHandle == nullptr) {
    return E_FAIL;
  }
  pImageFile = LockResource(imageResDataHandle);
  // Lock the resource and calculate the image's size
  if (pImageFile == nullptr) {
    // Lock it to get the system memory pointer
    return E_FAIL;
  }
  imageFileSize = SizeofResource(hInst, imageResHandle);
  if (imageFileSize == 0) {
    // Calculate the size
    return E_FAIL;
  }
  hr = pIWICFactory->CreateStream(&pStream);
  // Create an IWICStream object
  if (FAILED(hr)) {
    // Create a WIC stream to map onto the memory
    return hr;
  }
  hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE *>(pImageFile), imageFileSize);
  if (FAILED(hr)) {
    // Initialize the stream with the memory pointer and size
    return hr;
  }
  // Create a decoder for the stream
  hr =
      pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
  if (FAILED(hr)) {
    return hr;
  }
  hr = pDecoder->GetFrame(0, &pSource);
  // Retrieve a frame from the image and store it in an IWICBitmapFrameDecode object
  if (FAILED(hr)) {
    // Create the initial frame
    return hr;
  }

  // Before Direct2D can use the image, it must be converted to the 32bppPBGRA pixel format.
  // To convert the image format, use the IWICImagingFactory::CreateFormatConverter method to create
  // an IWICFormatConverter object, then use the IWICFormatConverter object's Initialize method to
  // perform the conversion.
  if (SUCCEEDED(hr)) {
    // Convert the image format to 32bppPBGRA
    // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
    hr = pIWICFactory->CreateFormatConverter(&pConverter);
  }

  if (SUCCEEDED(hr)) {
    // If a new width or height was specified, create and
    // IWICBitmapScaler and use it to resize the image.
    if (destinationWidth != 0 || destinationHeight != 0) {
      UINT originalWidth;
      UINT originalHeight;
      hr = pSource->GetSize(&originalWidth, &originalHeight);
      if (SUCCEEDED(hr)) {
        if (destinationWidth == 0) {
          FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
          destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
        } else if (destinationHeight == 0) {
          FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
          destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
        }
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
        if (SUCCEEDED(hr)) {
          hr = pScaler->Initialize(pSource, destinationWidth, destinationHeight,
                                   WICBitmapInterpolationModeCubic);
          if (SUCCEEDED(hr)) {
            hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA,
                                        WICBitmapDitherTypeNone, NULL, 0.f,
                                        WICBitmapPaletteTypeMedianCut);
          }
        }
      }
    } else {
      hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                  NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    }
  }

  // Finally, Create an ID2D1Bitmap object, that can be drawn by a render target and used with other
  // Direct2D objects
  if (SUCCEEDED(hr)) {
    // Create a Direct2D bitmap from the WIC bitmap
    hr = renderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
  }

  return hr;
}

MainWindow::~MainWindow() {
  Free(&writeTextFormat);
  Free(&writeFactory);
  Free(&textBrush);
  Free(&borderBrush);
  Free(&renderTarget);
  Free(&wicFactory);
  Free(&m_pFactory);
  if (hFont != nullptr) {
    DeleteFont(hFont);
  }
}

LRESULT MainWindow::InitializeWindow() {
  // change UI style
  hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(ICON_BAULK_BASE));
  bela::error_code ec;
  if (!InitializeBase(ec)) {
    bela::BelaMessageBox(nullptr, L"unable search baulk env", ec.message.data(), nullptr,
                         bela::mbs_t::FATAL);
    return S_FALSE;
  }
  if (CreateDeviceIndependentResources() != S_OK) {
    return S_FALSE;
  }
  RECT layout = {100, 100, 800, 320};
  Create(nullptr, layout, L"Baulk environment dock", noresizewnd,
         WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
  return S_OK;
}

///
HRESULT MainWindow::CreateDeviceIndependentResources() {
  HRESULT hr = S_OK;
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
  if (FAILED(hr)) {
    return hr;
  }
  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                           reinterpret_cast<IUnknown **>(&writeFactory));
  if (FAILED(hr)) {
    return hr;
  }
  hr = writeFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                      DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                      12.0f * 96.0f / 72.0f, L"zh-CN", &writeTextFormat);
  if (FAILED(hr)) {
    return hr;
  }
  hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
                        __uuidof(IWICImagingFactory2), reinterpret_cast<LPVOID *>(&wicFactory));
  return hr;
}

HRESULT MainWindow::CreateDeviceResources() {
  HRESULT hr = S_OK;
  if (renderTarget != nullptr) {
    return S_OK;
  }
  RECT rc;
  ::GetClientRect(m_hWnd, &rc);
  D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
  hr = m_pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                          D2D1::HwndRenderTargetProperties(m_hWnd, size),
                                          &renderTarget);
  renderTarget->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiX));
  if (SUCCEEDED(hr)) {
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &textBrush);
  }
  if (SUCCEEDED(hr)) {
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFC300), &borderBrush);
  }
  if (SUCCEEDED(hr)) {
    LoadResourceBitmap(hInst, renderTarget, wicFactory, MAKEINTRESOURCE(IMAGE_BAULK_BASE), L"PNG",
                       64, 64, &bitmap);
  }
  return hr;
}

void MainWindow::DiscardDeviceResources() {
  ///
  Free(&renderTarget);
  Free(&textBrush);
  Free(&borderBrush);
  Free(&bitmap);
}

HRESULT MainWindow::OnRender() {
  auto hr = CreateDeviceResources();
  if (FAILED(hr)) {
    return hr;
  }
  auto dsz = renderTarget->GetSize();
  renderTarget->BeginDraw();
  renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
  renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
  renderTarget->DrawRectangle(D2D1::RectF(20, 10, dsz.width - 20, dsz.height - 20), borderBrush,
                              1.0);

  renderTarget->DrawLine(D2D1::Point2F(20, 120), D2D1::Point2F(dsz.width - 20, 120), borderBrush,
                         1.0);
  if (bitmap != nullptr) {
    auto isz = bitmap->GetSize();
    renderTarget->DrawBitmap(bitmap, D2D1::RectF(60, 160, 60 + isz.width, 160 + isz.height), 1.0,
                             D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
  }
  for (const auto &label : labels) {
    if (label.empty()) {
      continue;
    }
    renderTarget->DrawTextW(label.data(), label.length(), writeTextFormat, label.F(), textBrush,
                            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                            DWRITE_MEASURING_MODE_NATURAL);
  }
  writeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  hr = renderTarget->EndDraw();

  if (hr == D2DERR_RECREATE_TARGET) {
    hr = S_OK;
    DiscardDeviceResources();
  }
  return hr;
}
D2D1_SIZE_U MainWindow::CalculateD2DWindowSize() {
  RECT rc;
  ::GetClientRect(m_hWnd, &rc);

  D2D1_SIZE_U d2dWindowSize = {0};
  d2dWindowSize.width = rc.right;
  d2dWindowSize.height = rc.bottom;

  return d2dWindowSize;
}

void MainWindow::OnResize(UINT width, UINT height) {
  if (renderTarget) {
    renderTarget->Resize(D2D1::SizeU(width, height));
  }
}

HRESULT MainWindow::InitializeControl() {
#ifdef _M_X64
  tables.Archs = {L"x64", L"arm64", L"x86", L"arm"};
#else
  tables.Archs = {L"x86", L"x64", L"arm64", L"arm"};
#endif
  for (const auto &a : tables.Archs) {
    ::SendMessageW(hvsarchbox.hWnd, CB_ADDSTRING, 0, (LPARAM)a.data());
  }
  ::SendMessageW(hvsarchbox.hWnd, CB_ADDSTRING, 0, (LPARAM)L"--none--");
  ::SendMessageW(hvsarchbox.hWnd, CB_SETCURSEL, 0, 0);
  for (const auto e : tables.Envs) {
    ::SendMessageW(hvenvbox.hWnd, CB_ADDSTRING, 0, (LPARAM)e.Desc.data());
  }
  ::SendMessageW(hvenvbox.hWnd, CB_ADDSTRING, 0, (LPARAM)L"--none--");
  ::SendMessageW(hvenvbox.hWnd, CB_SETCURSEL, static_cast<LPARAM>(tables.Envs.size()), 0);
  return S_OK;
}

bool RecreateFont(HFONT &hFont, int dpiY) {
  if (hFont == nullptr) {
    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  }
  LOGFONTW logFont = {0};
  if (GetObjectW(hFont, sizeof(logFont), &logFont) == 0) {
    return false;
  }
  logFont.lfHeight = -MulDiv(14, dpiY, 96);
  logFont.lfWeight = FW_NORMAL;
  wcscpy_s(logFont.lfFaceName, L"Segoe UI");
  auto hNewFont = CreateFontIndirectW(&logFont);
  if (hNewFont == nullptr) {
    return false;
  }
  DeleteObject(hFont);
  hFont = hNewFont;
  return true;
}

/*
 *  Message Action Function
 */
LRESULT MainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  // Adjust window initialize use real DPI
  dpiX = GetDpiForWindow(m_hWnd);
  dpiY = dpiX;
  RECT rect;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
  int cx = rect.right - rect.left;
  auto w = MulDiv(480, dpiX, 96);
  ::SetWindowPos(m_hWnd, nullptr, (cx - w) / 2, MulDiv(100, dpiX, 96), w, MulDiv(320, dpiX, 96),
                 SWP_NOZORDER | SWP_NOACTIVATE);
  RecreateFont(hFont, dpiY);
  SetIcon(hIcon, TRUE);
  //
  auto MakeWindow = [&](LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
                        int nWidth, int nHeight, HMENU hMenu, Widget &w) {
    auto hw = CreateWindowExW(wexstyle, lpClassName, lpWindowName, dwStyle, MulDiv(X, dpiX, 96),
                              MulDiv(Y, dpiY, 96), MulDiv(nWidth, dpiX, 96),
                              MulDiv(nHeight, dpiY, 96), m_hWnd, hMenu, hInst, nullptr);
    if (hw == nullptr) {
      return false;
    }
    w.hWnd = hw;
    w.layout.left = X;
    w.layout.top = Y;
    w.layout.right = X + nWidth;
    w.layout.bottom = Y + nHeight;
    ::SendMessageW(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    return true;
  };

  // combobox
  MakeWindow(WC_COMBOBOXW, L"", cbstyle, 180, 20, 240, 30, nullptr, hvsarchbox);
  MakeWindow(WC_COMBOBOXW, L"", cbstyle, 180, 55, 240, 30, nullptr, hvenvbox);

  // button
  MakeWindow(WC_BUTTONW, L"Make Cleanup Environment", chboxstyle, 180, 130, 240, 27, nullptr,
             hcleanenv);
  MakeWindow(WC_BUTTONW, L"Use Built-in Clang (VS)", chboxstyle, 180, 160, 240, 27, nullptr,
             hclang);
  MakeWindow(WC_BUTTONW, L"Open Baulk Terminal", pbstyle | BS_ICON, 180, 210, 240, 30,
             (HMENU)IDC_BUTTON_STARTENV, hbaulkenv);

  HMENU hSystemMenu = ::GetSystemMenu(m_hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_BAULK_DOCK_ABOUT,
              L"About Baulk environment dock\tAlt+F1");

  labels.emplace_back(30, 20, 180, 60, L"Visual Studio \U0001F19A"); //💻
  labels.emplace_back(30, 60, 180, 120, L"Virtual Env \U0001f6e0");  //⚙

  InitializeControl();
  return S_OK;
}

LRESULT MainWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  PostQuitMessage(0);
  return S_OK;
}

LRESULT MainWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  ::DestroyWindow(m_hWnd);
  return S_OK;
}

LRESULT MainWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  UINT width = LOWORD(lParam);
  UINT height = HIWORD(lParam);
  OnResize(width, height);
  return S_OK;
}

LRESULT MainWindow::OnDpiChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  dpiX = static_cast<UINT32>(LOWORD(wParam));
  dpiY = static_cast<UINT32>(HIWORD(wParam));
  auto prcNewWindow = reinterpret_cast<RECT *const>(lParam);
  // resize window with new DPI
  ::SetWindowPos(m_hWnd, nullptr, prcNewWindow->left, prcNewWindow->top,
                 prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
  RecreateFont(hFont, dpiY);
  renderTarget->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiY));
  auto UpdateWindowPos = [&](const Widget &w) {
    ::SetWindowPos(w.hWnd, NULL, MulDiv(w.layout.left, dpiX, 96), MulDiv(w.layout.top, dpiY, 96),
                   MulDiv(w.layout.right - w.layout.left, dpiX, 96),
                   MulDiv(w.layout.bottom - w.layout.top, dpiY, 96), SWP_NOZORDER | SWP_NOACTIVATE);
    ::SendMessageW(w.hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
  };
  UpdateWindowPos(hvsarchbox);
  UpdateWindowPos(hvenvbox);
  UpdateWindowPos(hcleanenv);
  UpdateWindowPos(hclang);
  UpdateWindowPos(hbaulkenv);
  return S_OK;
}

LRESULT MainWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  LRESULT hr = S_OK;
  PAINTSTRUCT ps;
  BeginPaint(&ps);
  /// if auto return OnRender(),CPU usage is too high
  hr = OnRender();
  EndPaint(&ps);
  return hr;
}

LRESULT MainWindow::OnCtlColorStatic(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  return S_OK;
}

LRESULT MainWindow::OnSysMemuAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled) {
  bela::BelaMessageBox(m_hWnd, L"About Baulk environment dock", BAULK_APPVERSION, BAULK_APPLINK,
                       bela::mbs_t::ABOUT);
  return S_OK;
}

} // namespace baulk::dock