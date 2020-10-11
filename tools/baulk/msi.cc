///
#include "baulk.hpp"
#include "fs.hpp"
#include <bela/numbers.hpp>
#include <bela/path.hpp>
#include <bela/terminal.hpp>
#include <Msi.h>

namespace baulk::msi {
// https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msisetexternaluiw
class Progressor {
public:
  Progressor(std::wstring_view msi) {
    if (auto nv = baulk::fs::FileName(msi); !nv.empty()) {
      name = nv;
    }
  }
  Progressor(const Progressor &) = delete;
  Progressor &operator=(const Progressor) = delete;
  void Update(int64_t total, int64_t rate);
  int Invoke(UINT iMessageType, LPCWSTR szMessage);

private:
  bool DoProgress(LPWSTR szMessage);
  bool Decode(LPWSTR szMessage);
  std::wstring name;
  int64_t field[4];
  int64_t mProgressTotal{0};
  int64_t mProgress{0};
  int64_t iCurPos;
  bool initialized{false};
  bool canceled{false};
  bool mEnableActionData{false};
  bool mForwardProgress{false};
  bool mScriptInProgress{false};
};

void Progressor::Update(int64_t total, int64_t rate) {
  if (!baulk::IsQuietMode && total != 0) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[32mmsi decompress %s: %d/%d\x1b[0m", name, rate, total);
  }
}

int64_t FGetInteger(wchar_t *&rpch) {
  wchar_t *pchPrev = rpch;
  while (*rpch && *rpch != ' ') {
    rpch++;
  }
  *rpch = '\0';
  int64_t i = 0;
  bela::SimpleAtoi(pchPrev, &i);
  return i;
}

bool Progressor::Decode(LPWSTR szMessage) {
  wchar_t *pch = szMessage;
  if (0 == *pch) {
    return false; // no msg
  }
  while (*pch != 0) {
    wchar_t chField = *pch++;
    pch++; // for ':'
    pch++; // for sp
    switch (chField) {
    case '1': // field 1
      // progress message type
      if (0 == isdigit(*pch)) {
        return false; // blank record
      }
      field[0] = *pch++ - '0';
      break;
    case '2': // field 2
      field[1] = FGetInteger(pch);
      if (field[0] == 2 || field[0] == 3) {
        return true; // done processing
      }
      break;
    case '3': // field 3
      field[2] = FGetInteger(pch);
      if (field[0] == 1) {
        return true; // done processing
      }
      break;
    case '4': // field 4
      field[3] = FGetInteger(pch);
      return true; // done processing
    default:       // unknown field
      return false;
    }
    pch++; // for space (' ') between fields
  }
  return true;
}

// https://docs.microsoft.com/zh-cn/windows/win32/api/msiquery/nf-msiquery-msiprocessmessage
bool Progressor::DoProgress(LPWSTR szMessage) {
  if (!Decode(szMessage)) {
    return false;
  }
  switch (field[0]) {
  case 0:
    mProgressTotal = field[1];
    mForwardProgress = (field[2] == 0);
    mProgress = (mForwardProgress != 0 ? 0 : mProgressTotal);
    Update(mProgressTotal, mScriptInProgress ? mProgressTotal : mProgress);
    iCurPos = 0;
    mScriptInProgress = (field[3] == 1);
  case 1:
    if (field[2] != 0) {
      mEnableActionData = true;
      break;
    }
    mEnableActionData = false;
    break;
  case 2:
    if (mProgressTotal != 0) {
      iCurPos += field[1];
      Update(mProgressTotal, mForwardProgress ? iCurPos : -1 * iCurPos);
      break;
    }
    break;
  }
  return true;
}

int Progressor::Invoke(UINT iMessageType, LPCWSTR szMessage) {
  if (!initialized) {
    MsiSetInternalUI(INSTALLUILEVEL_BASIC, nullptr);
    initialized = true;
  }
  if (canceled) {
    return IDCANCEL;
  }
  if (szMessage == nullptr) {
    return 0;
  }
  auto mt = static_cast<INSTALLMESSAGE>(0xFF000000 & iMessageType);
  auto uiflags = 0x00FFFFFF & iMessageType;

  switch (mt) {
  case INSTALLMESSAGE_FATALEXIT:
    bela::FPrintF(stderr, L"baulk decompress msi \x1b[31mFATALEXIT: %s\x1b[0m\n", szMessage);
    return IDABORT;
  case INSTALLMESSAGE_ERROR:
    bela::FPrintF(stderr, L"baulk decompress msi error: \x1b[31m%s\x1b[0m\n", szMessage);
    return IDABORT;
  case INSTALLMESSAGE_WARNING:
    bela::FPrintF(stderr, L"baulk decompress msi warning: \x1b[33m%s\x1b[0m\n", szMessage);
    // IDIGNORE
    break;
  case INSTALLMESSAGE_USER:
    return IDOK;
  case INSTALLMESSAGE_FILESINUSE:
    /* Display FilesInUse dialog */
    // parse the message text to provide the names of the
    // applications that the user can close so that the
    // files are no longer in use.
    return 0;

  case INSTALLMESSAGE_RESOLVESOURCE:
    /* ALWAYS return 0 for ResolveSource */
    return 0;

  case INSTALLMESSAGE_OUTOFDISKSPACE:
    /* Get user message here */
    return IDOK;

  case INSTALLMESSAGE_ACTIONSTART:
    /* New action started, any action data is sent by this new action */
    mEnableActionData = false;
    return IDOK;
  case INSTALLMESSAGE_PROGRESS:
    if (szMessage != nullptr) {
      DoProgress(const_cast<LPWSTR>(szMessage));
    }
    return IDOK;
  case INSTALLMESSAGE_INITIALIZE:
    return IDOK;
  // Sent after UI termination, no string data
  case INSTALLMESSAGE_TERMINATE:
    return IDOK;
  // Sent prior to display of authored dialog or wizard
  case INSTALLMESSAGE_SHOWDIALOG:
    return IDOK;
  default:
    break;
  }
  return IDOK;
}

// msi_ui_callback
INT WINAPI msi_ui_callback(LPVOID ctx, UINT iMessageType, LPCWSTR szMessage) {
  auto pg = reinterpret_cast<Progressor *>(ctx);
  return pg->Invoke(iMessageType, szMessage);
}

bool Decompress(std::wstring_view msi, std::wstring_view outdir, bela::error_code &ec) {
  Progressor pg(msi);
  auto cmd = bela::StringCat(L"ACTION=ADMIN TARGETDIR=\"", outdir, L"\"");
  MsiSetInternalUI(INSTALLUILEVEL(INSTALLUILEVEL_NONE | INSTALLUILEVEL_SOURCERESONLY), nullptr);
  // https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msisetexternaluiw
  MsiSetExternalUIW(msi_ui_callback,
                    INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING |
                        INSTALLLOGMODE_USER | INSTALLLOGMODE_INFO | INSTALLLOGMODE_RESOLVESOURCE |
                        INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART | INSTALLLOGMODE_ACTIONDATA |
                        INSTALLLOGMODE_COMMONDATA | INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_INITIALIZE |
                        INSTALLLOGMODE_TERMINATE | INSTALLLOGMODE_SHOWDIALOG,
                    &pg);
  if (MsiInstallProductW(msi.data(), cmd.data()) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return false;
  }
  if (!baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}

inline void DecompressClear(std::wstring_view dir) {
  constexpr std::wstring_view msiext = L".msi";
  std::error_code ec;
  for (auto &p : std::filesystem::directory_iterator(dir)) {
    auto extension = p.path().extension().wstring();
    if (bela::EqualsIgnoreCase(extension, msiext)) {
      baulk::DbgPrint(L"remove msi %s\n", p.path().c_str());
      std::filesystem::remove_all(p.path(), ec);
    }
  }
}

bool Regularize(std::wstring_view path) {
  DecompressClear(path);
  bela::error_code ec;
  baulk::fs::PathRemoveEx(bela::StringCat(path, L"\\Windows"), ec); //
  constexpr std::wstring_view destdirs[] = {L"\\Program Files", L"\\ProgramFiles64", L"\\PFiles", L"\\Files"};
  for (auto d : destdirs) {
    auto sd = bela::StringCat(path, d);
    if (!bela::PathExists(sd)) {
      continue;
    }
    if (baulk::fs::FlatPackageInitialize(sd, path, ec)) {
      bela::error_code ec2;
      baulk::fs::PathRemoveEx(sd, ec2);
      return !ec;
    }
  }
  return baulk::fs::FlatPackageInitialize(path, path, ec);
}

} // namespace baulk::msi