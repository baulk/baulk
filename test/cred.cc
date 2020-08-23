///
#include <windows.h>
#include <wincred.h>
#include <cstdio>
#include <clocale>
#include <string>

int wmain(int argc, wchar_t **argv) {
  setlocale(LC_ALL, "");
  wchar_t username[CREDUI_MAX_USERNAME_LENGTH + 1];
  wchar_t passwd[CREDUI_MAX_PASSWORD_LENGTH + 1];
  ZeroMemory(username, sizeof(username));
  ZeroMemory(passwd, sizeof(passwd));
  // wchar_t username2[CREDUI_MAX_USERNAME_LENGTH + 1];
  BOOL fSave = FALSE;
  auto ec = CredUICmdLinePromptForCredentialsW(L"BaulkApp", nullptr, 0, username, CREDUI_MAX_USERNAME_LENGTH, passwd,
                                               CREDUI_MAX_PASSWORD_LENGTH, &fSave,
                                               CREDUI_FLAGS_EXCLUDE_CERTIFICATES | CREDUI_FLAGS_DO_NOT_PERSIST);
  if (ec != NO_ERROR) {
    fwprintf(stderr, L"CredUICmdLinePromptForCredentialsW: %d\n", ec);
    return 1;
  }
  fwprintf(stderr, L"username: %s\npassword: %s\n", username, passwd);
  return 0;
}