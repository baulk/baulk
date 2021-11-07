#ifndef BELA_UI_LOADER_HPP
#define BELA_UI_LOADER_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/ascii.hpp>

namespace bela::windows::windows_internal {
class symbol_loader {
public:
  symbol_loader() { InitializeCriticalSection(&cs); }
  symbol_loader(const symbol_loader &) = delete;
  symbol_loader &operator=(const symbol_loader &) = delete;
  ~symbol_loader() {
    for (const auto &[k, m] : modules) {
      if (m != nullptr) {
        FreeLibrary(m);
      }
    }
    DeleteCriticalSection(&cs);
  }
  template <typename F>
  bool ensure_loaded(F &fn, std::wstring_view dllname, std::string_view func, bool loadFromSystem32 = false) noexcept {
    fn = reinterpret_cast<F>(ensure_loaded(dllname, func, loadFromSystem32));
    return fn != nullptr;
  }

  template <typename F> bool ensure_system_loaded(F &fn, std::wstring_view dllname, std::string_view func) noexcept {
    fn = reinterpret_cast<F>(ensure_system_loaded(dllname, func));
    return fn != nullptr;
  }

private:
  bela::flat_hash_map<std::wstring, HMODULE> modules;
  bela::flat_hash_map<std::wstring, HMODULE> system_modules;
  CRITICAL_SECTION cs;
  FARPROC ensure_loaded(std::wstring_view dllname, std::string_view func, bool loadFromSystem32) noexcept {
    EnterCriticalSection(&cs);
    auto leaver = bela::finally([&] { LeaveCriticalSection(&cs); });
    if (auto it = modules.find(dllname); it != modules.end() && it->second != nullptr) {
      return GetProcAddress(it->second, func.data());
    }
    HMODULE hModule = nullptr;
    if (loadFromSystem32) {
      hModule = LoadLibraryExW(dllname.data(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    } else {
      hModule = LoadLibraryW(dllname.data());
    }
    if (hModule == nullptr) {
      return nullptr;
    }
    modules.emplace(dllname, hModule);
    return GetProcAddress(hModule, func.data());
  }
  FARPROC ensure_system_loaded(std::wstring_view dllname, std::string_view func) noexcept {
    EnterCriticalSection(&cs);
    auto leaver = bela::finally([&] { LeaveCriticalSection(&cs); });
    if (auto it = system_modules.find(dllname); it != system_modules.end() && it->second != nullptr) {
      return GetProcAddress(it->second, func.data());
    }
    HMODULE hModule = GetModuleHandleW(dllname.data());
    if (hModule == nullptr) {
      return nullptr;
    }
    system_modules.emplace(dllname, hModule);
    return GetProcAddress(hModule, func.data());
  }
};

} // namespace bela::windows::windows_internal

#endif