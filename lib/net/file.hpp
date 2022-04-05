//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/time.hpp>
#include <bela/ascii.hpp>
#include <bela/io.hpp>
#include <filesystem>
#include <baulk/allocate.hpp>
#include <baulk/net/types.hpp>

namespace baulk::net::net_internal {
enum class hash_t : uint16_t {
  NONE = 0,
  SHA224 = 1, //
  SHA256,     //
  SHA384,     //
  SHA512,     //
  SHA3,       //
  SHA3_224,   //
  SHA3_256,   //
  SHA3_384,   //
  SHA3_512,   //
  BLAKE3
};

constexpr std::wstring_view part_suffix = L".part";
constexpr uint8_t part_magic[] = {'P', 'A', 'R', 'T'};
#pragma pack(push, 1)
struct part_overlay_data {
  uint8_t magic[4];
  hash_t method{hash_t::NONE};
  uint16_t hashsz{0};
  uint8_t hash[64];
  int64_t total_bytes{0};
  int64_t current_bytes{0};
  int64_t laste_time{0};
};
#pragma pack(pop)

struct HashPrefix {
  const std::wstring_view prefix;
  hash_t method;
  uint16_t hashsz;
};
constexpr HashPrefix hnmaps[] = {
    {L"BLAKE3", hash_t::BLAKE3, 32},     // BLAKE3
    {L"SHA224", hash_t::SHA224, 28},     // SHA224
    {L"SHA256", hash_t::SHA256, 32},     // SHA256
    {L"SHA384", hash_t::SHA384, 48},     // SHA384
    {L"SHA512", hash_t::SHA512, 64},     // SHA512
    {L"SHA3-224", hash_t::SHA3_224, 28}, // SHA3-224
    {L"SHA3-256", hash_t::SHA3_256, 32}, // SHA3-256
    {L"SHA3-384", hash_t::SHA3_384, 48}, // SHA3-384
    {L"SHA3-512", hash_t::SHA3_512, 64}, // SHA3-512
    {L"SHA3", hash_t::SHA3, 32},         // SHA3 alias for SHA3-256
};

inline bool hash_construct(std::wstring_view hash_value, part_overlay_data &overlay_data, bela::error_code &ec) {
  std::wstring_view value = hash_value;
  overlay_data.method = hash_t::SHA256;
  overlay_data.hashsz = 32;
  if (auto pos = hash_value.find(':'); pos != std::wstring_view::npos) {
    value = hash_value.substr(pos + 1);
    auto prefix = bela::AsciiStrToUpper(hash_value.substr(0, pos));
    auto fn = [&]() {
      for (const auto &h : hnmaps) {
        if (h.prefix == prefix) {
          overlay_data.method = h.method;
          overlay_data.hashsz = h.hashsz;
          return true;
        }
      }
      return false;
    };
    if (!fn()) {
      ec = bela::make_error_code(bela::ErrGeneral, L"unsupported hash method '", prefix, L"'");
      return false;
    }
  }
  memset(overlay_data.hash, 0, sizeof(overlay_data.hash));
  if (!hash_decode(value, overlay_data.hash)) {
    ec = bela::make_error_code(bela::ErrGeneral, L"unsupport hash text '", value, L"'");
    return false;
  }
  return true;
}

struct _File_disposition_info_ex {
  DWORD _Flags;
};

inline bool disposition_file_handle(HANDLE fd) {
  _File_disposition_info_ex _Info_ex{0x3};

  // FileDispositionInfoEx isn't documented in MSDN at the time of this writing, but is present
  // in minwinbase.h as of at least 10.0.16299.0
  constexpr auto _FileDispositionInfoExClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(21);
  if (SetFileInformationByHandle(reinterpret_cast<HANDLE>(fd), _FileDispositionInfoExClass, &_Info_ex,
                                 sizeof(_Info_ex))) {
    return true;
  }
  auto ec = bela::make_system_error_code(L"SetFileInformationByHandle() ");

  switch (ec.code) {
  case ERROR_INVALID_PARAMETER: // Older Windows versions
    [[fallthrough]];
  case ERROR_INVALID_FUNCTION: // Windows 10 1607
    [[fallthrough]];
  case ERROR_NOT_SUPPORTED: // POSIX delete not supported by the file system
    break;                  // try non-POSIX delete below
  case ERROR_ACCESS_DENIED: // This might be due to the read-only bit, try to clear it and try again
    [[fallthrough]];
  default:
    bela::FPrintF(stderr, L"Disposition: %s\n", ec);
    return false;
  }

  FILE_DISPOSITION_INFO _Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(reinterpret_cast<HANDLE>(fd), FileDispositionInfo, &_Info, sizeof(_Info))) {
    return true;
  }
  return false;
}

inline void cleanup_close_file(HANDLE fd) {
  disposition_file_handle(fd);
  CloseHandle(fd);
}

inline bool truncated_file(HANDLE fd, int64_t pos, bela::error_code &ec) {
  if (!bela::io::Seek(fd, pos, ec)) {
    return false;
  }
  if (SetEndOfFile(fd) != TRUE) {
    ec = bela::make_system_error_code(L"SetEndOfFile() ");
    return false;
  }
  return true;
}

class FilePart {
public:
  FilePart(HANDLE fd_, const std::filesystem::path &fsPath_, int64_t total_bytes_, int64_t current_bytes_,
           int64_t recent_)
      : fd(fd_), fsPath(fsPath_), total_bytes(total_bytes_), current_bytes(current_bytes_), laste_time(recent_) {}
  FilePart(const FilePart &) = delete;
  FilePart &operator=(const FilePart &) = delete;
  ~FilePart() noexcept { file_discard(); }
  void RenameTo(const std::filesystem::path &newPath) { fsPath = newPath; }
  auto FileSize() const { return total_bytes; }
  auto CurrentBytes() const { return current_bytes; }
  auto LasteTime() const { return bela::FromUnixSeconds(laste_time); }
  bool Truncated(bela::error_code &ec) {
    if (!truncated_file(fd, 0, ec)) {
      return false;
    }
    current_bytes = 0;
    total_bytes = 0;
    return true;
  }
  bool SaveOverlayData(std::wstring_view hash_value, int64_t total_bytes, int64_t current_bytes, bela::error_code &ec) {
    if (!discard_file_handle) {
      ec = bela::make_error_code(L"FilePart not a discard file");
      return false;
    }
    if (total_bytes <= 0 || current_bytes == 0) {
      ec = bela::make_error_code(L"Current download not support part download");
      return false;
    }
    auto now = bela::Now();
    part_overlay_data overlay_data{
        .magic = {'P', 'A', 'R', 'T'},
        .method = hash_t::NONE,
        .hashsz = {0},
        .hash = {0},
        .total_bytes = total_bytes,
        .current_bytes = current_bytes,
        .laste_time = bela::ToUnixSeconds(now),
    };
    if (!hash_construct(hash_value, overlay_data, ec)) {
      return false;
    }
    if (auto fileSize = bela::io::Size(fd, ec); fileSize != current_bytes) {
      ec = bela::make_error_code(L"FilePart size not equal current_bytes size");
      return false;
    }
    if (!bela::io::Seek(fd, current_bytes, ec)) {
      return false;
    }
    if (!WriteFull(overlay_data, ec)) {
      return false;
    }
    discard_file_handle = false;
    return true;
  }

  template <typename T> bool WriteFull(const T &t, bela::error_code &ec) { return WriteFull(&t, sizeof(t), ec); }
  bool WriteFull(const void *data, size_t bytes, bela::error_code &ec) {
    auto len = static_cast<DWORD>(bytes);
    auto u8d = reinterpret_cast<const uint8_t *>(data);
    DWORD writtenBytes = 0;
    do {
      DWORD dwSize = 0;
      if (WriteFile(fd, u8d + writtenBytes, len - writtenBytes, &dwSize, nullptr) != TRUE) {
        ec = bela::make_system_error_code(L"WriteFile() ");
        return false;
      }
      writtenBytes += dwSize;
    } while (writtenBytes < len);
    return true;
  }
  // solidified
  bool Solidified(bela::error_code &ec) {
    if (fd == INVALID_HANDLE_VALUE) {
      ec = bela::make_error_code(L"FilePart is invalid");
      return false;
    }
    const auto &nativePath = fsPath.native();
    auto roSize = sizeof(FILE_RENAME_INFO) + (nativePath.size() + 1) * sizeof(wchar_t);
    auto renameOptions = baulk::mem::make_unique_variable<FILE_RENAME_INFO>(roSize);
    if (!renameOptions) {
      ec = bela::make_error_code(L"allocate failed");
      return false;
    }
    renameOptions->ReplaceIfExists = TRUE;
    renameOptions->RootDirectory = nullptr;
    renameOptions->FileNameLength = static_cast<DWORD>((nativePath.size() + 1) * sizeof(wchar_t));
    memcpy(renameOptions->FileName, nativePath.data(), nativePath.size() * sizeof(wchar_t));
    renameOptions->FileName[nativePath.size()] = 0;
    if (SetFileInformationByHandle(fd, FileRenameInfo, renameOptions.get(), static_cast<DWORD>(roSize)) != TRUE) {
      ec = bela::make_system_error_code(L"SetFileInformationByHandle");
      return false;
    }
    CloseHandle(fd);
    fd = INVALID_HANDLE_VALUE;
    return true;
  }

  static std::optional<FilePart> MakeFilePart(const std::filesystem::path &p, std::wstring_view hash_value,
                                              bela::error_code &ec) {
    std::error_code e;
    auto fsPath = std::filesystem::absolute(p, e);
    auto part = bela::StringCat(fsPath.native(), part_suffix);
    // https://devblogs.microsoft.com/oldnewthing/20170310-00/?p=95705
    auto fd = ::CreateFileW(part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE | GENERIC_READ | GENERIC_WRITE | DELETE,
                            FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            nullptr);
    if (fd == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    auto fileSize = bela::io::Size(fd, ec);
    if (fileSize == -1) {
      cleanup_close_file(fd);
      return std::nullopt;
    }
    auto local_truncated = [&]() -> bool {
      if (fileSize == 0) {
        return true;
      }
      return truncated_file(fd, 0, ec);
    };
    if (fileSize <= static_cast<int64_t>(sizeof(part_overlay_data)) || hash_value.empty()) {
      if (!local_truncated()) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fd, fsPath, 0, 0, 0);
    }
    part_overlay_data overlayInput{
        .magic = {0},
        .method = hash_t::NONE,
        .hashsz = {0},
        .hash = {0},
        .total_bytes = 0,
        .current_bytes = 0,
        .laste_time = 0,
    };
    if (!hash_construct(hash_value, overlayInput, ec)) {
      if (!local_truncated()) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fd, fsPath, 0, 0, 0);
    }
    auto seekTo = fileSize - static_cast<int64_t>(sizeof(part_overlay_data));
    part_overlay_data overlayDisk{
        .magic = {0},
        .method = hash_t::NONE,
        .hashsz = {0},
        .hash = {0},
        .total_bytes = 0,
        .current_bytes = 0,
        .laste_time = 0,
    };
    size_t outSize = 0;
    if (!bela::io::ReadAt(fd, &overlayDisk, sizeof(overlayDisk), seekTo, outSize, ec)) {
      if (!local_truncated()) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fd, fsPath, 0, 0, 0);
    }
    if (!bytes_equal(overlayDisk.magic, part_magic) || !bytes_equal(overlayInput.hash, overlayDisk.hash) ||
        overlayDisk.method != overlayInput.method || overlayDisk.hashsz != overlayInput.hashsz) {
      if (!local_truncated()) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fd, fsPath, 0, 0, 0);
    }
    if (!truncated_file(fd, seekTo, ec)) {
      return std::nullopt;
    }
    // current_bytes part found
    return std::make_optional<FilePart>(fd, fsPath, overlayDisk.total_bytes, overlayDisk.current_bytes,
                                        overlayDisk.laste_time);
  }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  std::filesystem::path fsPath;
  int64_t total_bytes{0};
  int64_t current_bytes{0};
  int64_t laste_time{0};
  bool discard_file_handle{true};
  void file_discard() noexcept {
    if (fd != INVALID_HANDLE_VALUE) {
      if (discard_file_handle) {
        disposition_file_handle(fd);
      }
      CloseHandle(fd);
    }
  }
};

} // namespace baulk::net::net_internal