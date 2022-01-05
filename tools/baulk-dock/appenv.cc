///
#include "app.hpp"
//
#include <windowsx.h> // box help
#include <CommCtrl.h>
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <bela/picker.hpp>
#include <bela/env.hpp>
#include <bela/escapeargv.hpp>
#include <bela/io.hpp>
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include <filesystem>

namespace baulk::dock {
constexpr const wchar_t *string_nullable(std::wstring_view str) { return str.empty() ? nullptr : str.data(); }
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (!bela::PathExists(wt)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

inline constexpr bool DirSkipFaster(const wchar_t *dir) {
  return (dir[0] == L'.' && (dir[1] == L'\0' || (dir[1] == L'.' && dir[2] == L'\0')));
}

class Finder {
public:
  Finder() noexcept = default;
  Finder(const Finder &) = delete;
  Finder &operator=(const Finder &) = delete;
  ~Finder() noexcept {
    if (hFind != INVALID_HANDLE_VALUE) {
      FindClose(hFind);
    }
  }
  const WIN32_FIND_DATAW &FD() const { return wfd; }
  bool Ignore() const { return DirSkipFaster(wfd.cFileName); }
  bool IsDir() const { return (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  std::wstring_view Name() const { return std::wstring_view(wfd.cFileName); }
  bool Next() { return FindNextFileW(hFind, &wfd) == TRUE; }
  bool First(std::wstring_view dir, std::wstring_view suffix, bela::error_code &ec) {
    auto d = bela::StringCat(dir, L"\\", suffix);
    hFind = FindFirstFileW(d.data(), &wfd);
    if (hFind == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }

private:
  HANDLE hFind{INVALID_HANDLE_VALUE};
  WIN32_FIND_DATAW wfd;
};

bool SearchVirtualEnv(std::wstring_view lockfile, std::wstring_view pkgName, EnvNode &node) {
  bela::error_code ec;
  auto jo = json::parse_file(lockfile, ec);
  if (!jo) {
    return false;
  }
  auto jv = jo->view();
  auto version = jv.fetch("version");
  if (auto sv = jv.subview("venv"); sv) {
    node.Value = pkgName;
    node.Desc = pkgName;
    if (auto category = sv->fetch("category"); !category.empty()) {
      bela::StrAppend(&node.Desc, L"(", category, L")    ", version, L"");
    }
    return true;
  }
  return false;
}

bool MainWindow::InitializeBase(bela::error_code &ec) {
  if (!vfs::InitializeFastPathFs(ec)) {
    return false;
  }
  Finder finder;
  if (finder.First(vfs::AppLocks(), L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgname = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgname, L".json")) {
        continue;
      }
      auto lockfile = bela::StringCat(vfs::AppLocks(), L"\\", pkgname);
      pkgname.remove_suffix(5);
      baulk::dock::EnvNode node;
      if (SearchVirtualEnv(lockfile, pkgname, node)) {
        tables.AddVirtualEnv(std::move(node));
      }
    } while (finder.Next());
  }
  return true;
}

bool MainWindow::LoadPlacement(WINDOWPLACEMENT &placement) {
  auto posFilePath = bela::StringCat(vfs::AppData(), L"\\baulk\\baulk-dock.pos.json");
  bela::error_code ec;
  auto jo = json::parse_file(posFilePath, ec);
  if (!jo) {
    return false;
  }
  auto jv = jo->view();
  placement.flags = jv.fetch_as_integer("flags", 0u);
  placement.ptMaxPosition.x = jv.fetch_as_integer("ptMaxPosition.X", 0l);
  placement.ptMaxPosition.y = jv.fetch_as_integer("ptMaxPosition.Y", 0l);
  placement.ptMinPosition.x = jv.fetch_as_integer("ptMinPosition.X", 0l);
  placement.ptMinPosition.y = jv.fetch_as_integer("ptMinPosition.Y", 0l);
  placement.showCmd = jv.fetch_as_integer("showCmd", 0u);
  placement.rcNormalPosition.bottom = jv.fetch_as_integer("rcNormalPosition.bottom", 0l);
  placement.rcNormalPosition.left = jv.fetch_as_integer("rcNormalPosition.left", 0l);
  placement.rcNormalPosition.right = jv.fetch_as_integer("rcNormalPosition.right", 0l);
  placement.rcNormalPosition.top = jv.fetch_as_integer("rcNormalPosition.top", 0l);
  return true;
}
void MainWindow::SavePlacement(const WINDOWPLACEMENT &placement) {
  auto saveDir = bela::StringCat(vfs::AppData(), L"\\baulk");
  std::error_code e;
  if (std::filesystem::create_directories(saveDir, e); e) {
    return;
  }
  auto posfile = bela::StringCat(saveDir, L"\\baulk-dock.pos.json");
  nlohmann::json j;
  j["flags"] = placement.flags;
  j["ptMaxPosition.X"] = placement.ptMaxPosition.x;
  j["ptMaxPosition.Y"] = placement.ptMaxPosition.y;
  j["ptMinPosition.X"] = placement.ptMinPosition.x;
  j["ptMinPosition.Y"] = placement.ptMinPosition.y;
  j["showCmd"] = placement.showCmd;
  j["rcNormalPosition.bottom"] = placement.rcNormalPosition.bottom;
  j["rcNormalPosition.left"] = placement.rcNormalPosition.left;
  j["rcNormalPosition.right"] = placement.rcNormalPosition.right;
  j["rcNormalPosition.top"] = placement.rcNormalPosition.top;
  bela::error_code ec;
  bela::io::WriteTextAtomic(j.dump(4), posfile, ec);
}

template <size_t Len = 256> std::wstring GetCwd() {
  std::wstring s;
  s.resize(Len);
  auto len = GetCurrentDirectoryW(Len, s.data());
  if (len == 0) {
    return L"";
  }
  if (len < Len) {
    s.resize(len);
    return s;
  }
  s.resize(len);
  auto nlen = GetCurrentDirectoryW(len, s.data());
  if (nlen == 0 || nlen > len) {
    return L"";
  }
  s.resize(nlen);
  return s;
}

LRESULT MainWindow::OnStartupEnv(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled) {
  auto baulkexec = vfs::AppLocationPath(L"baulk-exec.exe");
  auto cwd = GetCwd();
  bela::EscapeArgv ea;
  if (auto wt = FindWindowsTerminal(); wt) {
    ea.Assign(*wt).Append(L"--title").Append(L"Windows Terminal \U0001F496 Baulk").Append(L"--");
  }
  ea.Append(baulkexec).Append(L"-W").Append(GetCwd());
  if (Button_GetCheck(hcleanenv.hWnd) == BST_CHECKED) {
    ea.Append(L"--cleanup");
  }
  if (auto index = ComboBox_GetCurSel(hvsarchbox.hWnd);
      index >= 0 && static_cast<size_t>(index) < tables.Archs.size()) {
    ea.Append(L"--vs").Append(L"-A").Append(tables.Archs[index]);
  }
  if (auto index = ComboBox_GetCurSel(hvenvbox.hWnd); index >= 0 && static_cast<size_t>(index) < tables.Envs.size()) {
    ea.Append(L"-E").Append(tables.Envs[index].Value);
  }
  ea.Append(L"winsh");
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si,
                     &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::BelaMessageBox(m_hWnd, L"unable open Windows Terminal", ec.data(), nullptr, bela::mbs_t::FATAL);
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  return S_OK;
}
} // namespace baulk::dock