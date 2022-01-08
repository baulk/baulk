///
#include <bela/simulator.hpp>
#include <bela/path.hpp>

namespace bela::env {

inline bool is_shell_specia_var(wchar_t ch) {
  return (ch == '*' || ch == '#' || ch == '$' || ch == '@' || ch == '!' || ch == '?' || ch == '-' ||
          (ch >= '0' && ch <= '9'));
}

inline bool is_alphanum(wchar_t ch) {
  return (ch == '_' || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

std::wstring_view resovle_shell_name(std::wstring_view s, size_t &off) {
  off = 0;
  if (s.front() == '{') {
    if (s.size() > 2 && is_shell_specia_var(s[1]) && s[2] == '}') {
      off = 3;
      return s.substr(1, 2);
    }
    for (size_t i = 1; i < s.size(); i++) {
      if (s[i] == '}') {
        if (i == 1) {
          off = 2;
          return L"";
        }
        off = i + 1;
        return s.substr(1, i - 1);
      }
    }
    off = 1;
    return L"";
  }
  if (is_shell_specia_var(s[0])) {
    off = 1;
    return s.substr(0, 1);
  }
  size_t i = 0;
  for (; i < s.size() && is_alphanum(s[i]); i++) {
    ;
  }
  off = i;
  return s.substr(0, i);
}

void ExpandEnvInternal(std::wstring_view raw, std::wstring &w) {
  std::wstring save;
  w.reserve(raw.size() * 2);
  size_t i = 0;
  for (size_t j = 0; j < raw.size(); j++) {
    if (raw[j] == '$' && j + 1 < raw.size()) {
      w.append(raw.substr(i, j - i));
      size_t off = 0;
      auto name = resovle_shell_name(raw.substr(j + 1), off);
      if (name.empty()) {
        if (off == 0) {
          w.push_back(raw[j]);
        }
      } else {
        if (bela::LookupEnv(std::wstring(name), save)) {
          w.append(save);
        }
      }
      j += off;
      i = j + 1;
    }
  }
  w.append(raw.substr(i));
}

std::wstring ExpandEnv(std::wstring_view raw) {
  std::wstring w;
  ExpandEnvInternal(raw, w);
  return w;
}

std::wstring PathExpand(std::wstring_view raw) {
  if (raw == L"~") {
    return GetEnv(L"USERPROFILE");
  }
  std::wstring s;
  if (bela::StartsWith(raw, L"~\\") || bela::StartsWith(raw, L"~/")) {
    raw.remove_prefix(2);
    s.assign(GetEnv(L"USERPROFILE")).push_back(L'\\');
  }
  ExpandEnvInternal(raw, s);
  // replace
  for (auto &c : s) {
    if (c == '/') {
      c = '\\';
    }
  }
  return s;
}

constexpr std::wstring_view env_wstrings[] = {
    L"ALLUSERSPROFILE",
    L"APPDATA",
    L"CommonProgramFiles",
    L"CommonProgramFiles(x86)",
    L"CommonProgramW6432",
    L"COMPUTERNAME",
    L"ComSpec",
    L"HOMEDRIVE",
    L"HOMEPATH",
    L"LOCALAPPDATA",
    L"LOGONSERVER",
    L"NUMBER_OF_PROCESSORS",
    L"OS",
    L"PATHEXT",
    L"PROCESSOR_ARCHITECTURE",
    L"PROCESSOR_ARCHITEW6432",
    L"PROCESSOR_IDENTIFIER",
    L"PROCESSOR_LEVEL",
    L"PROCESSOR_REVISION",
    L"ProgramData",
    L"ProgramFiles",
    L"ProgramFiles(x86)",
    L"ProgramW6432",
    L"PROMPT",
    L"PSModulePath",
    L"PUBLIC",
    L"SystemDrive",
    L"SystemRoot",
    L"TEMP",
    L"TMP",
    L"USERDNSDOMAIN",
    L"USERDOMAIN",
    L"USERDOMAIN_ROAMINGPROFILE",
    L"USERNAME",
    L"USERPROFILE",
    L"windir",
    // Windows Terminal
    L"SESSIONNAME",
    L"WT_SESSION",
    L"WSLENV",
    // Enables proxy information to be passed to Curl, the underlying download
    // library in cmake.exe
    L"http_proxy",
    L"https_proxy",
    // Environment variables to tell git to use custom SSH executable or command
    L"GIT_SSH",
    L"GIT_SSH_COMMAND",
    // Environment variables needed for ssh-agent based authentication
    L"SSH_AUTH_SOCK",
    L"SSH_AGENT_PID",
    // Enables find_package(CUDA) and enable_language(CUDA) in CMake
    L"CUDA_PATH",
    L"CUDA_PATH_V9_0",
    L"CUDA_PATH_V9_1",
    L"CUDA_PATH_V10_0",
    L"CUDA_PATH_V10_1",
    L"CUDA_PATH_V10_2",
    L"CUDA_PATH_V11_0",
    L"CUDA_PATH_V11_1",
    L"CUDA_PATH_V11_2",
    // Environmental variable generated automatically by CUDA after installation
    L"NVCUDASAMPLES_ROOT",
    // Enables find_package(Vulkan) in CMake. Environmental variable generated
    // by Vulkan SDK installer
    L"VULKAN_SDK",
    // Enable targeted Android NDK
    L"ANDROID_NDK_HOME",
};

inline bool ExistsEnv(std::wstring_view k) {
  for (const auto s : env_wstrings) {
    if (bela::EqualsIgnoreCase(s, k)) {
      return true;
    }
  }
  return false;
}

bool cleanupPathExt(std::wstring_view pathext, std::vector<std::wstring> &exts) {
  constexpr std::wstring_view defaultexts[] = {L".com", L".exe", L".bat", L".cmd"};
  std::vector<std::wstring_view> exts_ = bela::StrSplit(pathext, bela::ByChar(bela::Separator), bela::SkipEmpty()); //
  if (exts_.empty()) {
    exts.assign(std::begin(defaultexts), std::end(defaultexts));
    return true;
  }
  for (const auto &e : exts_) {
    if (e.front() != '.') {
      exts.emplace_back(bela::StringCat(L".", bela::AsciiStrToLower(e)));
      continue;
    }
    exts.emplace_back(bela::AsciiStrToLower(e));
  }
  return true;
}

inline bool HasExt(std::wstring_view file) {
  if (auto pos = file.rfind(L'.'); pos != std::wstring_view::npos) {
    return file.find_last_of(L":\\/") < pos;
  }
  return false;
}

bool FindExecutable(std::wstring_view file, const std::vector<std::wstring> &exts, std::wstring &p) {
  if (HasExt(file) && PathFileIsExists(file)) {
    p = file;
    return true;
  }
  std::wstring newfile;
  newfile.reserve(file.size() + 8);
  newfile.assign(file);
  auto rawsize = newfile.size();
  for (const auto &e : exts) {
    // rawsize always < newfile.size();
    // std::char_traits::assign
    newfile.resize(rawsize);
    newfile.append(e);
    if (PathFileIsExists(newfile)) {
      p.assign(std::move(newfile));
      return true;
    }
  }
  return false;
}

bool Simulator::InitializeEnv() {
  LPWCH envs{nullptr};
  auto deleter = bela::finally([&] {
    if (envs != nullptr) {
      FreeEnvironmentStringsW(envs);
      envs = nullptr;
    }
  });
  envs = ::GetEnvironmentStringsW();
  if (envs == nullptr) {
    return false;
  }
  for (wchar_t const *lastch{envs}; *lastch != '\0'; ++lastch) {
    const auto len = ::wcslen(lastch);
    const std::wstring_view entry{lastch, len};
    const auto pos = entry.find(L'=');
    if (pos == std::wstring_view::npos) {
      lastch += len;
      continue;
    }
    auto key = entry.substr(0, pos);
    auto val = entry.substr(pos + 1);
    lastch += len;
    if (bela::EqualsIgnoreCase(key, L"Path")) {
      std::vector<std::wstring_view> paths_ = bela::StrSplit(val, bela::ByChar(bela::Separator), bela::SkipEmpty());
      for (const auto p : paths_) {
        paths.emplace_back(bela::PathCat(p));
      }
      continue;
    }
    if (bela::EqualsIgnoreCase(key, L"PATHEXT")) {
      cleanupPathExt(val, pathexts);
      continue;
    }
    envmap.emplace(key, val);
  }
  return true;
}

bool Simulator::InitializeCleanupEnv() {
  LPWCH envs{nullptr};
  auto deleter = bela::finally([&] {
    if (envs != nullptr) {
      FreeEnvironmentStringsW(envs);
      envs = nullptr;
    }
  });
  envs = ::GetEnvironmentStringsW();
  if (envs == nullptr) {
    return false;
  }
  for (wchar_t const *lastch{envs}; *lastch != '\0'; ++lastch) {
    const auto len = ::wcslen(lastch);
    const std::wstring_view entry{lastch, len};
    const auto pos = entry.find(L'=');
    if (pos == std::wstring_view::npos) {
      lastch += len;
      continue;
    }
    auto key = entry.substr(0, pos);
    auto val = entry.substr(pos + 1);
    lastch += len;
    if (bela::EqualsIgnoreCase(key, L"PATHEXT")) {
      cleanupPathExt(val, pathexts);
      continue;
    }
    if (ExistsEnv(key)) {
      envmap.emplace(key, val);
    }
  }
  bela::MakePathEnv(paths);
  return true;
}

bool Simulator::LookPath(std::wstring_view cmd, std::wstring &exe, bool absPath) const {
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, pathexts, exe);
  }
  if (!absPath) {
    auto cwdfile = bela::PathAbsolute(cmd);
    if (FindExecutable(cwdfile, pathexts, exe)) {
      return true;
    }
  }
  for (const auto& p : paths) {
    auto exefile = bela::StringCat(p, L"\\", cmd);
    if (FindExecutable(exefile, pathexts, exe)) {
      return true;
    }
  }
  return false;
}

void Simulator::PathOrganize() {
  cachedEnv.clear();
  bela::flat_hash_set<std::wstring, bela::env::StringCaseInsensitiveHash, bela::env::StringCaseInsensitiveEq> sets;
  std::vector<std::wstring> newpaths;
  sets.reserve(paths.size());
  for (auto &p : paths) {
    auto s = bela::PathCat(p);
    if (sets.find(s) != sets.end()) {
      continue;
    }
    sets.emplace(s);
    newpaths.emplace_back(std::move(s));
  }
  paths = std::move(newpaths);
}

bool Simulator::ExpandEnv(std::wstring_view raw, std::wstring &w) const {
  w.reserve(w.size() + raw.size() * 2);
  size_t i = 0;
  for (size_t j = 0; j < raw.size(); j++) {
    if (raw[j] == '$' && j + 1 < raw.size()) {
      w.append(raw.substr(i, j - i));
      size_t off = 0;
      auto name = resovle_shell_name(raw.substr(j + 1), off);
      if (name.empty()) {
        if (off == 0) {
          w.push_back(raw[j]);
        }
      } else {
        if (auto it = envmap.find(name); it != envmap.end()) {
          w.append(it->second);
        }
      }
      j += off;
      i = j + 1;
    }
  }
  w.append(raw.substr(i));
  return true;
}

bool LookPath(std::wstring_view cmd, std::wstring &exe, const std::vector<std::wstring> &paths, bool absPath) {
  std::vector<std::wstring> exts;
  cleanupPathExt(bela::GetEnv(L"PATHEXT"), exts);
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, exts, exe);
  }
  if (!absPath) {
    auto cwdfile = bela::PathAbsolute(cmd);
    if (FindExecutable(cwdfile, exts, exe)) {
      return true;
    }
  }
  for (const auto& p : paths) {
    auto exefile = bela::StringCat(p, L"\\", cmd);
    if (FindExecutable(exefile, exts, exe)) {
      return true;
    }
  }
  return false;
}

bool LookPath(std::wstring_view cmd, std::wstring &exe, bool absPath) {
  std::vector<std::wstring> exts;
  cleanupPathExt(bela::GetEnv(L"PATHEXT"), exts);
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, exts, exe);
  }
  if (!absPath) {
    auto cwdfile = bela::PathAbsolute(cmd);
    if (FindExecutable(cwdfile, exts, exe)) {
      return true;
    }
  }
  auto path = GetEnv<4096>(L"PATH"); // 4K suggest.
  std::vector<std::wstring_view> pathv = bela::StrSplit(path, bela::ByChar(L';'), bela::SkipEmpty());
  for (auto p : pathv) {
    auto exefile = bela::StringCat(p, L"\\", cmd);
    if (FindExecutable(exefile, exts, exe)) {
      return true;
    }
  }
  return false;
}

} // namespace bela::env