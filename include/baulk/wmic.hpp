#ifndef BAULK_WMIC_HPP
#define BAULK_WMIC_HPP
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
#include <bela/str_join.hpp>
#include <bela/terminal.hpp>
#include <bela/time.hpp>
#include <bela/win32.hpp>
#include <bela/env.hpp>
#include <bela/comutils.hpp>
#include <bela/str_replace.hpp>
#include <dxgi.h>
#include <format>
#include <comdef.h>
#include <Wbemidl.h>
#include <propvarutil.h>
#include <thread>

namespace baulk::wmic {

struct wmic_locator {
public:
  wmic_locator() = default;
  bool connect(const wchar_t *strNetworkResource, bela::error_code &ec) {
    if (CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                         reinterpret_cast<void **>(&locator)) != S_OK) {
      ec = bela::make_system_error_code(L"Failed to create IWbemLocator object. ");
      return false;
    }
    auto result = locator->ConnectServer(_bstr_t(strNetworkResource), // Object path of WMI namespace
                                         NULL,                        // User name. NULL = current user
                                         NULL,                        // User password. NULL = current
                                         0,                           // Locale. NULL indicates current
                                         NULL,                        // Security flags.
                                         0,                           // Authority (for example, Kerberos)
                                         0,                           // Context object
                                         &services                    // pointer to IWbemServices proxy
    );
    if (FAILED(result)) {
      ec = bela::make_system_error_code(L"connect wmic error ");
      return false;
    }
    result = CoSetProxyBlanket(services.get(),              // Indicates the proxy to set
                               RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                               RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                               NULL,                        // Server principal name
                               RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
                               RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                               NULL,                        // client identity
                               EOAC_NONE                    // proxy capabilities
    );
    if (FAILED(result)) {
      ec = bela::make_system_error_code(L"CoSetProxyBlanket ");
      return false;
    }
    return true;
  }
  auto get() const { return services.get(); }

private:
  wmic_locator(const wmic_locator &) = delete;
  wmic_locator &operator=(const wmic_locator &) = delete;
  bela::comptr<IWbemLocator> locator;
  bela::comptr<IWbemServices> services;
};

// https://learn.microsoft.com/zh-cn/windows-hardware/drivers/storage/msft-physicaldisk
enum disk_media_t : std::uint16_t { Unspecified = 0, HDD = 3, SSD = 4, SCM = 5 };
constexpr std::wstring_view disk_media_name(disk_media_t m) {
  switch (m) {
  case HDD:
    return L"HDD";
  case SSD:
    return L"SSD";
  case SCM:
    return L"SCM";
  }
  return L"Unspecified";
}

constexpr std::wstring_view disk_bus_type(std::uint32_t i) {
  switch (i) {
  case 1: // SCSI
    return L"SCSI";
  case 2:
    return L"ATAPI";
  case 3:
    return L"ATA";
  case 4:
    return L"1394";
  case 5:
    return L"SSA";
  case 6:
    return L"Fibre Channel";
  case 7:
    return L"USB";
  case 8:
    return L"RAID";
  case 9:
    return L"iSCSI";
  case 10:
    return L"SAS";
  case 11:
    return L"SATA";
  case 12:
    return L"SD";
  case 13:
    return L"MMC"; //	Multimedia Card (MMC)
  case 14:
    return L"MAX"; // This value is reserved for system use.
  case 15:
    return L"File Backed Virtual";
  case 16:
    return L"Storage Spaces";
  case 17:
    return L"NVMe";
  case 18:
    return L"Microsoft Reserved";
  default:
    break;
  }
  return L"Unknown";
}

struct disk_drive {
  std::wstring friendly_name;
  std::wstring device_id;
  std::wstring bus_type;
  std::uint64_t size{0};
  disk_media_t m;
  std::wstring_view media_name() const { return disk_media_name(m); }
};

struct disk_drives {
public:
  std::vector<disk_drive> drives;
  // query_msft_physical_disk: https://learn.microsoft.com/zh-cn/windows-hardware/drivers/storage/msft-physicaldisk
  bool initialize(bela::error_code &ec) {
    wmic_locator locator;
    // ROOT\\microsoft\\windows\\storage
    if (!locator.connect(L"ROOT\\microsoft\\windows\\storage", ec)) {
      return false;
    }
    auto s = locator.get();
    bela::comptr<IEnumWbemClassObject> e;
    auto result = s->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM MSFT_PhysicalDisk"),
                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &e);

    if (FAILED(result)) {
      return false;
    }
    // MSFT_PhysicalDisk
    ULONG uReturn = 0;
    while (e) {
      bela::comptr<IWbemClassObject> clsObj;
      HRESULT hr = e->Next(WBEM_INFINITE, 1, &clsObj, &uReturn);
      if (0 == uReturn) {
        break;
      }
      bela::unique_variant FriendlyName;
      bela::unique_variant DeviceID;
      bela::unique_variant BusType;
      bela::unique_variant Size;
      bela::unique_variant mediaType;
      hr = clsObj->Get(L"FriendlyName", 0, &FriendlyName, 0, 0);
      hr = clsObj->Get(L"DeviceID", 0, &DeviceID, 0, 0);
      hr = clsObj->Get(L"BusType", 0, &BusType, 0, 0);
      hr = clsObj->Get(L"Size", 0, &Size, 0, 0);
      hr = clsObj->Get(L"MediaType", 0, &mediaType, 0, 0);
      std::uint16_t m{0};
      VariantToUInt16(*(mediaType.addressof()), &m);
      std::uint64_t size{0};
      VariantToUInt64(*(Size.addressof()), &size);
      drives.emplace_back(disk_drive{                                                          //
                                     .friendly_name = FriendlyName.bstrVal,                    //
                                     .device_id = DeviceID.bstrVal,                            //
                                     .bus_type = std::wstring(disk_bus_type(BusType.uintVal)), //
                                     .size = size,                                             //
                                     .m = static_cast<disk_media_t>(m)});
    }
    return true;
    ;
  }
};

inline std::optional<disk_drives> search_disk_drives(bela::error_code &ec) {
  disk_drives ds;
  if (!ds.initialize(ec)) {
    return std::nullopt;
  }
  return std::make_optional<>(std::move(ds));
}
} // namespace baulk::wmic

#endif