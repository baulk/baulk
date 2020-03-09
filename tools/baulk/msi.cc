///
#include "baulk.hpp"
#include "fs.hpp"
#include <bela/numbers.hpp>
#include <bela/path.hpp>
#include <bela/stdwriter.hpp>
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
  void Update(UINT total, UINT rate);
  int Invoke(UINT iMessageType, LPCWSTR szMessage);

private:
  bool DoProgress(std::wstring_view msg);
  bool Decode(std::wstring_view msg);
  std::wstring name;
  int field[4];
  int mProgressTotal{0};
  int mProgress{0};
  int iCurPos;
  bool initialized{false};
  bool canceled{false};
  bool mEnableActionData{false};
  bool mForwardProgress{false};
  bool mScriptInProgress{false};
};

void Progressor::Update(UINT total, UINT rate) {
  bela::FPrintF(stderr, L"\rDecompress %s: %d/%d\n", name, rate, total);
}

int FGetInteger(const wchar_t *&it, const wchar_t *end) {
  auto pchPrev = it;
  while (it < end && *it != ' ') {
    it++;
  }
  std::wstring_view sv{pchPrev, static_cast<size_t>(it - pchPrev)};
  int i = 0;
  bela::SimpleAtoi(sv, &i);
  return i;
}

bool Progressor::Decode(std::wstring_view msg) {
  if (msg.empty()) {
    return false;
  }
  auto it = msg.data();
  auto end = it + msg.size();
  while (it < end) {
    auto ch = *it;
    it += 2;
    if (it >= end) {
      return false;
    }
    switch (ch) {
    case '1':
      if (isdigit(ch) == 0) {
        return false;
      }
      field[0] = *it++ - '0';
      break;
    case '2':
      field[1] = FGetInteger(it, end);
      if (field[0] == 2 || field[0] == 3) {
        return true;
      }
      break;
    case 3:
      field[2] = FGetInteger(it, end);
      if (field[0] == 1) {
        return true;
      }
    case '4':
      field[3] = FGetInteger(it, end);
      return true;
    default:
      return false;
    }
    it++;
  }
  return true;
}

// https://docs.microsoft.com/zh-cn/windows/win32/api/msiquery/nf-msiquery-msiprocessmessage
bool Progressor::DoProgress(std::wstring_view msg) {
  if (!Decode(msg)) {
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
    return IDABORT;
  case INSTALLMESSAGE_ERROR:
    return IDABORT;
  case INSTALLMESSAGE_WARNING:
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
      DoProgress(szMessage);
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

bool Decompress(std::wstring_view msi, std::wstring_view outdir,
                bela::error_code &ec) {
  Progressor pg(msi);
  auto cmd = bela::StringCat(L"ACTION=ADMIN TARGETDIR=\"", outdir, L"\"");
  MsiSetInternalUI(
      INSTALLUILEVEL(INSTALLUILEVEL_NONE | INSTALLUILEVEL_SOURCERESONLY),
      nullptr);
  // https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msisetexternaluiw
  MsiSetExternalUIW(
      msi_ui_callback,
      INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_FATALEXIT |
          INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_USER |
          INSTALLLOGMODE_INFO | INSTALLLOGMODE_RESOLVESOURCE |
          INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART |
          INSTALLLOGMODE_ACTIONDATA | INSTALLLOGMODE_COMMONDATA |
          INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_INITIALIZE |
          INSTALLLOGMODE_TERMINATE | INSTALLLOGMODE_SHOWDIALOG,
      &pg);
  if (MsiInstallProductW(msi.data(), cmd.data()) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

inline void DecompressClear(std::wstring_view dir) {
  std::error_code ec;
  for (auto &p : std::filesystem::directory_iterator(dir)) {
    if (p.path().extension() == L".msi") {
      std::filesystem::remove_all(p.path(), ec);
    }
  }
}

bool Regularize(std::wstring_view path) {
  DecompressClear(path);
  bela::error_code ec;
  baulk::fs::PathRemove(bela::StringCat(path, L"\\Windows"), ec); //
  constexpr std::wstring_view destdirs[] = {
      L"\\Program Files", L"\\ProgramFiles64", L"\\PFiles", L"\\Files"};
  for (auto d : destdirs) {
    auto sd = bela::StringCat(path, d);
    if (!bela::PathExists(sd)) {
      continue;
    }
    if (baulk::fs::UniqueSubdirMoveTo(sd, path, ec)) {
      return !ec;
    }
  }
  return true;
}

} // namespace baulk::msi