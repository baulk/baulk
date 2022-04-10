#ifndef BAULK_ARCHIVE_MSI_HPP
#define BAULK_ARCHIVE_MSI_HPP
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <Msi.h>
#include <functional>
#include <filesystem>
#include <baulk/fs.hpp>
#include <bela/terminal.hpp>

namespace baulk::archive::msi {
enum MessageLevel { MessageFatal = 0, MessageError = 1, MessageWarn = 2 };

class Dispatcher {
public:
  using progressor = std::function<bool(int64_t extracted, int64_t total)>;
  using reporter = std::function<void(std::wstring_view, MessageLevel)>;
  Dispatcher() = default;
  Dispatcher(const Dispatcher &) = delete;
  Dispatcher &operator=(const Dispatcher &) = delete;
  void Assign(progressor &&p, reporter &&r) {
    progress_ = std::move(p);
    reporter_ = std::move(r);
  }
  void operator()(std::wstring_view message, MessageLevel level) {
    if (reporter_) {
      reporter_(message, level);
    }
  }
  bool operator()(int64_t extracted, int64_t total) {
    if (progress_) {
      return progress_(extracted, total);
    }
    return true;
  }

private:
  progressor progress_;
  reporter reporter_;
};

class Extractor {
public:
  Extractor() = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool Initialize(const std::filesystem::path &file, const std::filesystem::path &dest,
                  std::function<void(Dispatcher &)> delayInitializer, bela::error_code &ec) {
    std::error_code e;
    if (archive_file = std::filesystem::canonical(file, e); e) {
      ec = e;
      return false;
    }
    if (destination = std::filesystem::absolute(dest, e); e) {
      ec = e;
      return false;
    }
    if (delayInitializer) {
      delayInitializer(dispatcher);
    }
    return true;
  }
  bool Extract(bela::error_code &ec) {
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
  bool MakeFlattened(bela::error_code &ec) {
    std::error_code e;
    // Drop msi package
    constexpr std::wstring_view extension = L".msi";
    for (const auto &entry : std::filesystem::directory_iterator{destination, e}) {
      if (bela::EqualsIgnoreCase(entry.path().extension().native(), extension)) {
        std::filesystem::remove_all(entry.path(), e);
      }
    }

    auto overflow = [&](const std::filesystem::path &child) {
      for (const auto &entry : std::filesystem::directory_iterator{child, e}) {
        auto newPath = destination / entry.path().filename();
        bela::FPrintF(stderr, L"move %v to %v\n", entry.path(), newPath);
        std::filesystem::rename(entry.path(), newPath, e);
      }
    };
    // remove some child folder
    constexpr std::wstring_view childLists[] = {L"Program Files", L"ProgramFiles64", L"PFiles", L"Files"};
    for (const auto c : childLists) {
      auto child = destination / c;
      if (!std::filesystem::exists(child, e)) {
        continue;
      }
      overflow(child);
      bela::FPrintF(stderr, L"remove %v\n", child);
      std::filesystem::remove_all(child, e);
    }
    return baulk::fs::MakeFlattened(destination, ec);
  }

private:
  static INT WINAPI extract_callback(LPVOID ctx, UINT iMessageType, LPCWSTR szMessage) {
    if (szMessage == nullptr) {
      return 0;
    }
    return reinterpret_cast<Extractor *>(ctx)->extract_callback(iMessageType, szMessage);
  }
  int extract_callback(UINT iMessageType, LPCWSTR szMessage);
  bool parse_progress_message(LPWSTR szMessage);
  bool decode_progress_message(LPWSTR szMessage);
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  Dispatcher dispatcher;
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

inline int64_t FGetInteger(wchar_t *&rpch) {
  wchar_t *pchPrev = rpch;
  while (*rpch != 0U && *rpch != ' ') {
    rpch++;
  }
  *rpch = '\0';
  int64_t i = 0;
  (void)bela::SimpleAtoi(pchPrev, &i);
  return i;
}

inline bool Extractor::decode_progress_message(LPWSTR szMessage) {
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
inline bool Extractor::parse_progress_message(LPWSTR szMessage) {
  if (!decode_progress_message(szMessage)) {
    return false;
  }
  switch (field[0]) {
  case 0:
    mProgressTotal = field[1];
    mForwardProgress = (field[2] == 0);
    mProgress = (mForwardProgress ? 0 : mProgressTotal);
    dispatcher(mScriptInProgress ? mProgressTotal : mProgress, mProgressTotal);
    iCurPos = 0;
    mScriptInProgress = (field[3] == 1);
    break;
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
      dispatcher(mForwardProgress ? iCurPos : -1 * iCurPos, mProgressTotal);
      break;
    }
    break;
  }
  return true;
}

inline int Extractor::extract_callback(UINT iMessageType, LPCWSTR szMessage) {
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
  switch (mt) {
  case INSTALLMESSAGE_FATALEXIT:
    dispatcher(szMessage, MessageFatal);
    return IDABORT;
  case INSTALLMESSAGE_ERROR:
    dispatcher(szMessage, MessageError);
    return IDABORT;
  case INSTALLMESSAGE_WARNING:
    dispatcher(szMessage, MessageWarn);
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
      parse_progress_message(const_cast<LPWSTR>(szMessage));
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

} // namespace baulk::archive::msi

#endif