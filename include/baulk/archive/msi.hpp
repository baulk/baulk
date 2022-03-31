#ifndef BAULK_ARCHIVE_MSI_HPP
#define BAULK_ARCHIVE_MSI_HPP
#include <bela/base.hpp>
#include <Msi.h>
#include <functional>
#include <filesystem>
#include <baulk/fs.hpp>

namespace baulk::archive::msi {
using OnProgress = std::function<bool(int64_t extracted, int64_t total)>;
class Extractor {
public:
  Extractor() = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool Initialize(const std::filesystem::path &file, const std::filesystem::path &dest, bela::error_code &ec) {
    std::error_code e;
    if (archive_file = std::filesystem::canonical(file, e); e) {
      ec = e;
      return false;
    }
    if (destination = std::filesystem::absolute(dest, e); e) {
      ec = e;
      return false;
    }
    return true;
  }
  bool Extract(OnProgress &&pg, bela::error_code &ec) {
    progress = std::move(pg);
    auto actionArgument = bela::StringCat(L"ACTION=ADMIN TARGETDIR=\"", destination.native(), L"\"");
    MsiSetInternalUI(INSTALLUILEVEL(INSTALLUILEVEL_NONE | INSTALLUILEVEL_SOURCERESONLY), nullptr);
    // https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msisetexternaluiw
    MsiSetExternalUIW(Extractor::extract_callback,
                      INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR |
                          INSTALLLOGMODE_WARNING | INSTALLLOGMODE_USER | INSTALLLOGMODE_INFO |
                          INSTALLLOGMODE_RESOLVESOURCE | INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART |
                          INSTALLLOGMODE_ACTIONDATA | INSTALLLOGMODE_COMMONDATA | INSTALLLOGMODE_PROGRESS |
                          INSTALLLOGMODE_INITIALIZE | INSTALLLOGMODE_TERMINATE | INSTALLLOGMODE_SHOWDIALOG,
                      this);
    if (MsiInstallProductW(archive_file.c_str(), actionArgument.data()) != ERROR_SUCCESS) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }

private:
  static INT WINAPI extract_callback(LPVOID ctx, UINT iMessageType, LPCWSTR szMessage) {
    if (szMessage == nullptr) {
      return 0;
    }
    auto extractor = reinterpret_cast<Extractor *>(ctx);
    if (!extractor->progress) {
      return 0;
    }
    return extractor->extract_callback(iMessageType, szMessage);
  }
  int extract_callback(UINT iMessageType, LPCWSTR szMessage) {
    if (!initialized) {
      MsiSetInternalUI(INSTALLUILEVEL_BASIC, nullptr);
      initialized = true;
    }
    if (canceled) {
      return IDCANCEL;
    }
    return 0;
  }
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  OnProgress progress;
  int64_t field[4];
  int64_t mProgressTotal{0};
  int64_t mProgress{0};
  int64_t iCurPos;
  std::atomic_bool initialized{false};
  std::atomic_bool canceled{false};
  bool mEnableActionData{false};
  bool mForwardProgress{false};
  bool mScriptInProgress{false};
};
} // namespace baulk::archive::msi

#endif