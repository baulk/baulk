///
#include <bela/base.hpp>
#include <bela/escapeargv.hpp>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <Shobjidl.h>
#include <ShlObj.h>
#include <wrl.h>
#include <wrl/module.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <filesystem>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
  }
  return TRUE;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
inline HINSTANCE GetModuleInstanceHandle() { return reinterpret_cast<HINSTANCE>(&__ImageBase); }

std::optional<std::wstring> LookupModuleBasePath(HMODULE hModule, bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(MAX_PATH);
  for (;;) {
    const auto blen = buffer.size();
    auto n = GetModuleFileNameW(hModule, buffer.data(), static_cast<DWORD>(blen));
    if (n == 0) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    buffer.resize(n);
    if (n < static_cast<DWORD>(blen)) {
      break;
    }
  }
  return std::make_optional(std::move(buffer));
}

class __declspec(uuid("803A733E-9F73-4F38-BB43-1005CEC3B19D")) OpenBaulkTerminalHere
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom | Microsoft::WRL::InhibitFtmBase>,
          IExplorerCommand, IObjectWithSite> {
public:
  // IExplorerCommand
  IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray *items, _Outptr_result_nullonfailure_ PWSTR *ppszTitle) {
    return SHStrDup(L"Open Baulk Termianl Here", ppszTitle);
  }
  IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray *, _Outptr_result_nullonfailure_ PWSTR *ppszIcon) {
    constexpr std::wstring_view BaulkExe{L"baulk.exe"};
    bela::error_code ec;
    auto basePath = LookupModuleBasePath(reinterpret_cast<HMODULE>(GetModuleInstanceHandle()), ec);
    if (!basePath) {
      return 0;
    }
    std::filesystem::path modulePath{*basePath};
    modulePath.replace_filename(BaulkExe);
    return SHStrDupW(modulePath.c_str(), ppszIcon);
  }

  IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray *, _Outptr_result_nullonfailure_ PWSTR *ppszInfoTip) {
    *ppszInfoTip = nullptr;
    return E_NOTIMPL;
  }

  IFACEMETHODIMP GetCanonicalName(_Out_ GUID *guidCommandName) {
    *guidCommandName = __uuidof(this);
    return S_OK;
  }

  HRESULT GetState(IShellItemArray *psiItemArray, BOOL /*fOkToBeSlow*/, EXPCMDSTATE *pCmdState) {
    // compute the visibility of the verb here, respect "fOkToBeSlow" if this is
    // slow (does IO for example) when called with fOkToBeSlow == FALSE return
    // E_PENDING and this object will be called back on a background thread with
    // fOkToBeSlow == TRUE

    // We however don't need to bother with any of that, so we'll just return
    // ECS_ENABLED.

    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (GetBestLocationFromSelectionOrSite(psiItemArray, psi.GetAddressOf()) != S_OK) {
      return false;
    }

    SFGAOF attributes;
    const bool isFileSystemItem = psi && (psi->GetAttributes(SFGAO_FILESYSTEM, &attributes) == S_OK);
    *pCmdState = isFileSystemItem ? ECS_ENABLED : ECS_HIDDEN;
    *pCmdState = ECS_ENABLED;
    return S_OK;
  }

  IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray *psiItemArray, _In_opt_ IBindCtx *) noexcept {
    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (GetBestLocationFromSelectionOrSite(psiItemArray, psi.GetAddressOf()) != S_OK) {
      return false;
    }
    if (!psi) {
      return S_FALSE;
    }
    std::wstring path;
    LPWSTR pszName;
    if (!SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszName))) {
      return S_FALSE;
    }
    path = pszName;
    CoTaskMemFree(pszName);
    bela::error_code ec;
    auto basePath = LookupModuleBasePath(reinterpret_cast<HMODULE>(GetModuleInstanceHandle()), ec);
    if (!basePath) {
      return S_FALSE;
    }
    std::filesystem::path modulePath{*basePath};
    auto parent = modulePath.parent_path();
    auto baulkTerminal = parent.parent_path() / L"baulk-terminal.exe";
    std::error_code e;
    if (!std::filesystem::exists(baulkTerminal, e)) {
      baulkTerminal = parent / L"baulk-terminal.exe";
      if (!std::filesystem::exists(baulkTerminal, e)) {
        return S_FALSE;
      }
    }
    bela::EscapeArgv ea;
    ea.Assign(baulkTerminal.native()).Append(L"--vs").Append(L"-W").Append(path);
    PROCESS_INFORMATION pi;
    STARTUPINFOEXW siEx{0};
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEX);

    if (CreateProcessW(baulkTerminal.c_str(), // lpApplicationName
                       ea.data(),
                       nullptr,                                                   // lpProcessAttributes
                       nullptr,                                                   // lpThreadAttributes
                       false,                                                     // bInheritHandles
                       EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
                       nullptr,                                                   // lpEnvironment
                       path.data(),
                       &siEx.StartupInfo, // lpStartupInfo
                       &pi                // lpProcessInformation
                       ) != TRUE) {
      return S_FALSE;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return S_OK;
  }

  HRESULT GetFlags(EXPCMDFLAGS *pFlags) {
    *pFlags = ECF_DEFAULT;
    return S_OK;
  }

  HRESULT EnumSubCommands(IEnumExplorerCommand **ppEnum) {
    *ppEnum = nullptr;
    return E_NOTIMPL;
  }

  IFACEMETHODIMP SetSite(_In_ IUnknown *site) noexcept {
    site_ = site;
    return S_OK;
  }
  IFACEMETHODIMP GetSite(_In_ REFIID riid, _COM_Outptr_ void **site) noexcept {
    //
    return site_.CopyTo(riid, site);
  }

protected:
  Microsoft::WRL::ComPtr<IUnknown> site_;

private:
  HRESULT GetLocationFromSite(IShellItem **location) const noexcept;
  HRESULT GetBestLocationFromSelectionOrSite(IShellItemArray *psiArray, IShellItem **location) const noexcept;
};

HRESULT OpenBaulkTerminalHere::GetLocationFromSite(IShellItem **location) const noexcept {
  Microsoft::WRL::ComPtr<IServiceProvider> serviceProvider;
  if (site_.As(&serviceProvider) != S_OK) {
    return S_FALSE;
  }
  Microsoft::WRL::ComPtr<IFolderView> folderView;
  if (serviceProvider->QueryService(SID_SFolderView, folderView.GetAddressOf()) != S_OK) {
    return S_FALSE;
  }
  return folderView->GetFolder(IID_PPV_ARGS(location));
}

HRESULT OpenBaulkTerminalHere::GetBestLocationFromSelectionOrSite(IShellItemArray *psiArray,
                                                                  IShellItem **location) const noexcept {
  Microsoft::WRL::ComPtr<IShellItem> psi;
  if (psiArray) {
    DWORD count{};
    if (psiArray->GetCount(&count) != S_OK) {
      return S_FALSE;
    }
    if (count) // Sometimes we get an array with a count of 0. Fall back to the site chain.
    {
      if (psiArray->GetCount(&count) != S_OK) {
        return S_FALSE;
      }
    }
  }

  if (!psi) {
    if (GetLocationFromSite(&psi) != S_OK) {
      return S_FALSE;
    }
  }
  if (!psi) {
    return S_FALSE;
  }
  return psi.CopyTo(location);
}

CoCreatableClass(OpenBaulkTerminalHere) CoCreatableClassWrlCreatorMapInclude(OpenBaulkTerminalHere); //

STDAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ IActivationFactory **factory) {
  return Microsoft::WRL::Module<Microsoft::WRL::ModuleType::InProc>::GetModule().GetActivationFactory(
      activatableClassId, factory);
}

STDAPI DllCanUnloadNow() {
  return Microsoft::WRL::Module<Microsoft::WRL::InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ void **instance) {
  return Microsoft::WRL::Module<Microsoft::WRL::InProc>::GetModule().GetClassObject(rclsid, riid, instance);
}
