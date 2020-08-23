#ifndef BELA_PICKER_HPP
#define BELA_PICKER_HPP
#include "base.hpp"
#include <optional>
#include <Shlwapi.h>

namespace bela {
// Messagebox type
enum class mbs_t {
  INFO, //
  WARN,
  FATAL,
  ABOUT
};

bool BelaMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent, LPCWSTR pszExpandedInfo,
                    mbs_t mt = mbs_t::ABOUT);
using filter_t = COMDLG_FILTERSPEC;
std::optional<std::wstring> FilePicker(HWND hWnd, const wchar_t *title, const filter_t *fts, size_t flen);
template <size_t N>
inline std::optional<std::wstring> FilePicker(HWND hWnd, const wchar_t *title, const filter_t (&fts)[N]) {
  return FilePicker(hWnd, title, fts, N);
}
std::optional<std::wstring> FolderPicker(HWND hWnd, const wchar_t *title);
} // namespace bela

#endif