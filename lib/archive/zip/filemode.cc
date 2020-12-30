#include "zipinternal.hpp"

namespace baulk::archive::zip {
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
    mode |= ModeDevice;
    break;
  case s_IFCHR:
    mode |= ModeDevice | ModeCharDevice;
    break;
  case s_IFDIR:
    mode |= ModeDir;
    break;
  case s_IFIFO:
    mode |= ModeNamedPipe;
    break;
  case s_IFLNK:
    mode |= ModeSymlink;
    break;
  case s_IFREG:
    break;
  case s_IFSOCK:
    mode |= ModeSocket;
    break;
  default:
    break;
  }
  if ((m & s_ISGID) != 0) {
    mode |= ModeSetgid;
  }
  if ((m & s_ISUID) != 0) {
    mode |= ModeSetuid;
  }
  if ((m & s_ISVTX) != 0) {
    mode |= ModeSticky;
  }
  return static_cast<FileMode>(mode);
}

FileMode msdosModeToFileMode(uint32_t m) {
  uint32_t mode = 0;
  if ((m & msdosDir) != 0) {
    mode = ModeDir | 0777;
  } else {
    mode = 0666;
  }
  if ((m & msdosReadOnly) != 0) {
    mode &= ~0222;
  }
  return static_cast<FileMode>(mode);
}

baulk::archive::FileMode File::FileMode() const {
  auto mode = static_cast<baulk::archive::FileMode>(0);
  auto n = cversion >> 8;
  if (n == creatorUnix || n == creatorMacOSX) {
    mode = unixModeToFileMode(externalAttrs >> 16);
  } else if (n == creatorNTFS || n == creatorVFAT || n == creatorFAT) {
    mode = msdosModeToFileMode(externalAttrs);
  }
  if (name.ends_with('/')) {
    mode = mode | ModeDir;
  }
  return mode;
}
} // namespace baulk::archive::zip