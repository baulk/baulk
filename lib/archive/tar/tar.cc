//
#include "tarinternal.hpp"

namespace baulk::archive::tar {

FileReader::~FileReader() {
  if (fd != INVALID_HANDLE_VALUE && needClosed) {
    CloseHandle(fd);
  }
}
ssize_t FileReader::Read(void *buffer, size_t len, bela::error_code &ec) {
  DWORD drSize = {0};
  if (::ReadFile(fd, buffer, static_cast<DWORD>(len), &drSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return -1;
  }
  return static_cast<ssize_t>(drSize);
}
ssize_t FileReader::ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) {
  if (!PositionAt(pos, ec)) {
    return -1;
  }
  return Read(buffer, len, ec);
}
bool FileReader::PositionAt(int64_t pos, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx: ");
    return false;
  }
  return true;
}

std::shared_ptr<FileReader> OpenFile(std::wstring_view file, bela::error_code &ec) {
  auto fd = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return nullptr;
  }
  return std::make_shared<FileReader>(fd, true);
}

inline bool IsZero(const ustar_header &th) {
  static ustar_header zeroth = {0};
  return memcmp(&th, &zeroth, 0) == 0;
}

bela::ssize_t Reader::Read(void *buffer, size_t size, bela::error_code &ec) {
  if (r == nullptr) {
    return -1;
  }
  auto n = r->Read(buffer, size, ec);
  if (n > 0 && n <= unconsumedSize) {
    unconsumedSize -= static_cast<int64_t>(n);
  }
  return n;
}

// ReadAtLeast
bela::ssize_t Reader::ReadAtLeast(void *buffer, size_t size, bela::error_code &ec) {
  size_t rbytes = 0;
  auto p = reinterpret_cast<uint8_t *>(buffer);
  while (rbytes < size) {
    auto sz = Read(p + rbytes, size - rbytes, ec);
    if (sz < 0) {
      return -1;
    }
    if (sz == 0) {
      break;
    }
    rbytes += sz;
  }
  return rbytes;
}

bool Reader::discard(bela::error_code &ec) {
  constexpr int64_t dbsize = 4096;
  uint8_t disbuf[4096];
  while (unconsumedSize != 0) {
    auto minsize = (std::min)(unconsumedSize, dbsize);
    auto n = Read(disbuf, minsize, ec);
    if (n <= 0) {
      return false;
    }
  }
  return true;
}

std::optional<File> Reader::Next(bela::error_code &ec) {
  if (!discard(ec)) {
    return std::nullopt;
  }
  for (;;) {
    auto n = ReadAtLeast(&uhdr, sizeof(uhdr), ec);
    if (n < 0) {
      return std::nullopt;
    }
    if (n != sizeof(uhdr)) {
      ec = bela::make_error_code(L"invalid tar header");
      return std::nullopt;
    }
    if (uhdr.name[0] != 0 || !IsZero(uhdr)) {
      break;
    }
  }
  if (memcmp(uhdr.magic, magicUSTAR, sizeof(magicUSTAR)) == 0 &&
      memcmp(uhdr.version, versionUSTAR, sizeof(versionUSTAR)) == 0) {
    // USTAR
  }
  if (memcmp(uhdr.magic, magicGNU, sizeof(magicGNU)) == 0 &&
      memcmp(uhdr.version, versionGNU, sizeof(versionGNU)) == 0) {
    // GNU TAR
  }
  return std::nullopt;
}

} // namespace baulk::archive::tar