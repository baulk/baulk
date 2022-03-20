///
#include <string>
#include "zipinternal.hpp"

namespace hazel::zip {
using bela::os::FileMode;
std::string String(FileMode m) {
  constexpr const char str[] = "dalTLDpSugct?";
  char buf[32] = {0}; //
  int w = 0;
  for (int i = 0; i < 13; i++) {
    if ((m & (1 << (32 - 1 - i))) != 0) {
      buf[w] = str[i];
      w++;
    }
  }
  if (w == 0) {
    buf[w] = '-';
    w++;
  }
  constexpr const char rwx[] = "rwxrwxrwx";
  for (int i = 0; i < 11; i++) {
    if ((m & (1 << (9 - 1 - i))) != 0) {
      buf[w] = rwx[i];
    } else {
      buf[w] = '-';
    }
    w++;
  }
  return std::string(buf, w);
}

FileMode unixModeToFileMode(uint32_t m) {
  uint32_t mode = static_cast<FileMode>(m & 0777);
  switch (m & s_IFMT) {
  case s_IFBLK:
    mode |= FileMode::ModeDevice;
    break;
  case s_IFCHR:
    mode |= FileMode::ModeDevice | FileMode::ModeCharDevice;
    break;
  case s_IFDIR:
    mode |= FileMode::ModeDir;
    break;
  case s_IFIFO:
    mode |= FileMode::ModeNamedPipe;
    break;
  case s_IFLNK:
    mode |= FileMode::ModeSymlink;
    break;
  case s_IFREG:
    break;
  case s_IFSOCK:
    mode |= FileMode::ModeSocket;
    break;
  default:
    break;
  }
  if ((m & s_ISGID) != 0) {
    mode |= FileMode::ModeSetgid;
  }
  if ((m & s_ISUID) != 0) {
    mode |= FileMode::ModeSetuid;
  }
  if ((m & s_ISVTX) != 0) {
    mode |= FileMode::ModeSticky;
  }
  return static_cast<FileMode>(mode);
}

FileMode msdosModeToFileMode(uint32_t m) {
  uint32_t mode = 0;
  if ((m & msdosDir) != 0) {
    mode = FileMode::ModeDir | 0777;
  } else {
    mode = 0666;
  }
  if ((m & msdosReadOnly) != 0) {
    mode &= ~0222;
  }
  return static_cast<FileMode>(mode);
}

FileMode resolveFileMode(const File &file, uint32_t externalAttrs) {
  auto mode = static_cast<FileMode>(0);
  auto n = file.creator_version >> 8;
  if (n == creatorUnix || n == creatorMacOSX) {
    mode = unixModeToFileMode(externalAttrs >> 16);
  } else if (n == creatorNTFS || n == creatorVFAT || n == creatorFAT) {
    mode = msdosModeToFileMode(externalAttrs);
  }
  if (file.name.ends_with('/') || file.name.ends_with('\\')) {
    mode = mode | FileMode::ModeDir;
  }
  return mode;
}
} // namespace hazel::zip