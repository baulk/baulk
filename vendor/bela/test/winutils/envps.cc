#include <bela/simulator.hpp>
#include <bela/process.hpp>
#include <bela/terminal.hpp>

int LinkToApp(wchar_t *env) {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  wchar_t cmd[256] = L"pwsh";
  if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, env, nullptr, &si, &pi)) {
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  WaitForSingleObject(pi.hProcess, INFINITE);
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  return exitCode;
}

int GoVersion(bela::env::Simulator *simulator) {
  bela::process::Process p(simulator);
  p.Execute(L"go", L"version");
  return 0;
}
int CMDSet(bela::env::Simulator *simulator) {
  bela::process::Process p(simulator);
  p.Execute(L"cmd", L"/c", L"set");
  return 0;
}

int wmain() {
  bela::env::Simulator simulator;
  simulator.InitializeCleanupEnv();
  simulator.SetEnv(L"GOPROXY", L"https://goproxy.io/");
  simulator.PathAppend(L"C:\\Go\\bin");
  simulator.PathAppend(L"C:\\Go\\bin");
  auto p = simulator.PathExpand(L"~/.ssh/id_ed25519.pub");
  bela::FPrintF(stderr, L"[%s] Find ssh key: \x1b[32m%s\x1b[0m\n", simulator.GetEnv(L"USERPROFILE"), p);
  GoVersion(&simulator);
  CMDSet(&simulator);
  simulator.PathOrganize();
  CMDSet(&simulator);
  // simulator.SetEnv(L"Path", L"C:/Dev");
  auto envs = simulator.MakeEnv();
  return LinkToApp(envs.data());
}