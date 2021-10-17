//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/time.hpp>
#include <bela/ascii.hpp>
#include <bela/io.hpp>

namespace baulk::net::net_internal {
enum class hash_t : uint32_t {
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
  uint8_t hash[64];
  int64_t total_bytes{0};
  int64_t current_bytes{0};
  int64_t laste_time{0};
};
#pragma pack(pop)

inline constexpr int hex_digit_to_int(wchar_t c) {
  static_assert('0' == 0x30 && 'A' == 0x41 && 'a' == 0x61, "Character set must be ASCII.");
  // assert(bela::ascii_isxdigit(c));
  int x = static_cast<unsigned char>(c);
  if (x > '9') {
    x += 9;
  }
  return x & 0xf;
}

struct HashPrefix {
  const std::wstring_view prefix;
  hash_t method;
};
constexpr HashPrefix hnmaps[] = {
    {L"BLAKE3", hash_t::BLAKE3},     // BLAKE3
    {L"SHA224", hash_t::SHA224},     // SHA224
    {L"SHA256", hash_t::SHA256},     // SHA256
    {L"SHA384", hash_t::SHA384},     // SHA384
    {L"SHA512", hash_t::SHA512},     // SHA512
    {L"SHA3-224", hash_t::SHA3_224}, // SHA3-224
    {L"SHA3-256", hash_t::SHA3_256}, // SHA3-256
    {L"SHA3-384", hash_t::SHA3_384}, // SHA3-384
    {L"SHA3-512", hash_t::SHA3_512}, // SHA3-512
    {L"SHA3", hash_t::SHA3},         // SHA3 alias for SHA3-256
};

inline bool hash_construct(std::wstring_view hash_value, part_overlay_data &overlay_data, bela::error_code &ec) {

  std::wstring_view value = hash_value;
  overlay_data.method = hash_t::SHA256;
  if (auto pos = hash_value.find(':'); pos != std::wstring_view::npos) {
    value = hash_value.substr(pos + 1);
    auto prefix = bela::AsciiStrToUpper(hash_value.substr(0, pos));
    auto fn = [&]() {
      for (const auto &h : hnmaps) {
        if (h.prefix == prefix) {
          overlay_data.method = h.method;
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
  auto hlen = value.size() / 2;
  for (size_t i = 0; i < (std::min)(hlen, sizeof(overlay_data.hash)); i++) {
    auto a = hex_digit_to_int(value[i * 2]);
    auto b = hex_digit_to_int(value[i * 2 + 1]);
    overlay_data.hash[i] = static_cast<uint8_t>((a << 4) | b);
  }
  return true;
}

struct _File_disposition_info_ex {
  DWORD _Flags;
};

inline bool disposition_file_handle(HANDLE fh) {
  _File_disposition_info_ex _Info_ex{0x3};

  // FileDispositionInfoEx isn't documented in MSDN at the time of this writing, but is present
  // in minwinbase.h as of at least 10.0.16299.0
  constexpr auto _FileDispositionInfoExClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(21);
  if (SetFileInformationByHandle(reinterpret_cast<HANDLE>(fh), _FileDispositionInfoExClass, &_Info_ex,
                                 sizeof(_Info_ex))) {
    return true;
  }

  switch (GetLastError()) {
  case ERROR_INVALID_PARAMETER: // Older Windows versions
    [[fallthrough]];
  case ERROR_INVALID_FUNCTION: // Windows 10 1607
    [[fallthrough]];
  case ERROR_NOT_SUPPORTED: // POSIX delete not supported by the file system
    break;                  // try non-POSIX delete below
  case ERROR_ACCESS_DENIED: // This might be due to the read-only bit, try to clear it and try again
    [[fallthrough]];
  default:
    return false;
  }

  FILE_DISPOSITION_INFO _Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(reinterpret_cast<HANDLE>(fh), FileDispositionInfo, &_Info, sizeof(_Info))) {
    return true;
  }
  return false;
}

inline void cleanup_close_file(HANDLE fh) {
  disposition_file_handle(fh);
  CloseHandle(fh);
}

inline bool truncated_file(HANDLE fh, int64_t pos, bela::error_code &ec) {
  if (!bela::io::Seek(fh, pos, ec)) {
    return false;
  }
  if (!SetEndOfFile(fh) != TRUE) {
    ec = bela::make_system_error_code(L"SetEndOfFile() ");
    return false;
  }
  return true;
}

template <typename T, typename U, size_t N> bool ArrayEqual(const T (&a)[N], const U (&b)[N]) {
  return std::equal(a, a + N, b);
}

class FilePart {
public:
  FilePart(HANDLE fh, std::wstring &&p, int64_t total_bytes_, int64_t current_bytes_, int64_t recent_)
      : fileHandle(fh), path(std::move(p)), total_bytes(total_bytes_), current_bytes(current_bytes_),
        laste_time(recent_) {}
  FilePart(const FilePart &) = delete;
  FilePart &operator=(const FilePart &) = delete;
  ~FilePart() noexcept { file_discard(); }
  void RenameTo(std::wstring_view filename) { path = filename; }
  int64_t FileSize() const { return total_bytes; }
  int64_t CurrentBytes() const { return current_bytes; }
  bela::Time LasteTime() const { return bela::FromUnixSeconds(laste_time); }
  std::wstring_view FileName() const { return path; }
  bool Truncated(bela::error_code &ec) {
    if (!truncated_file(fileHandle, 0, ec)) {
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
        .hash = {0},
        .total_bytes = total_bytes,
        .current_bytes = current_bytes,
        .laste_time = bela::ToUnixSeconds(now),
    };
    if (!hash_construct(hash_value, overlay_data, ec)) {
      return false;
    }
    if (auto fileSize = bela::io::Size(fileHandle, ec); fileSize != current_bytes) {
      ec = bela::make_error_code(L"FilePart size not equal current_bytes size");
      return false;
    }
    if (!bela::io::Seek(fileHandle, current_bytes, ec)) {
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
      if (WriteFile(fileHandle, u8d + writtenBytes, len - writtenBytes, &dwSize, nullptr) != TRUE) {
        ec = bela::make_system_error_code(L"WriteFile() ");
        return false;
      }
      writtenBytes += dwSize;
    } while (writtenBytes < len);
    return true;
  }

  bool UtilizeFile(bela::error_code &ec) {
    if (fileHandle == INVALID_HANDLE_VALUE) {
      ec = bela::make_error_code(L"FilePart is invalid");
      return false;
    }
    auto fiSize = sizeof(FILE_RENAME_INFO) + (path.size() + 1) * sizeof(wchar_t);
    std::vector<uint8_t> buffer;
    try {
      buffer.resize(fiSize);
    } catch (const std::exception &e) {
      ec = bela::make_error_code(bela::encode_into<char, wchar_t>(e.what()));
      return false;
    }
    auto fi = reinterpret_cast<FILE_RENAME_INFO *>(buffer.data());
    fi->ReplaceIfExists = TRUE;
    fi->RootDirectory = nullptr;
    fi->FileNameLength = static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t));
    wmemcpy(fi->FileName, path.data(), path.size());
    fi->FileName[path.size()] = 0;
    if (SetFileInformationByHandle(fileHandle, FileRenameInfo, fi, static_cast<DWORD>(fiSize)) != TRUE) {
      ec = bela::make_system_error_code(L"SetFileInformationByHandle");
      return false;
    }
    CloseHandle(fileHandle);
    fileHandle = INVALID_HANDLE_VALUE;
    return true;
  }

  static std::optional<FilePart> MakeFilePart(std::wstring_view p, std::wstring_view hash_value, bela::error_code &ec) {
    auto pp = bela::PathAbsolute(p);
    auto part = bela::StringCat(pp, part_suffix);
    auto fh = ::CreateFileW(part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE, FILE_SHARE_READ, nullptr,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (fh == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    auto fileSize = bela::io::Size(fh, ec);
    if (fileSize == -1) {
      cleanup_close_file(fh);
      return std::nullopt;
    }
    if (fileSize <= static_cast<int64_t>(sizeof(part_overlay_data)) || hash_value.empty()) {
      if (truncated_file(fh, 0, ec)) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fh, std::move(pp), 0, 0, 0);
    }
    part_overlay_data overlayInput{
        .magic = {0},
        .method = hash_t::NONE,
        .hash = {0},
        .total_bytes = 0,
        .current_bytes = 0,
        .laste_time = 0,
    };
    if (!hash_construct(hash_value, overlayInput, ec)) {
      if (truncated_file(fh, 0, ec)) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fh, std::move(pp), 0, 0, 0);
    }
    auto seekTo = fileSize - static_cast<int64_t>(sizeof(part_overlay_data));
    part_overlay_data overlayDisk{
        .magic = {0},
        .method = hash_t::NONE,
        .hash = {0},
        .total_bytes = 0,
        .current_bytes = 0,
        .laste_time = 0,
    };
    size_t outSize = 0;
    if (!bela::io::ReadAt(fh, &overlayDisk, sizeof(overlayDisk), seekTo, outSize, ec)) {
      if (truncated_file(fh, 0, ec)) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fh, std::move(pp), 0, 0, 0);
    }
    if (!ArrayEqual(overlayDisk.magic, part_magic) || !ArrayEqual(overlayInput.hash, overlayDisk.hash) ||
        overlayDisk.method != overlayInput.method) {
      if (truncated_file(fh, 0, ec)) {
        return std::nullopt;
      }
      return std::make_optional<FilePart>(fh, std::move(pp), 0, 0, 0);
    }
    if (truncated_file(fh, seekTo, ec)) {
      return std::nullopt;
    }
    // current_bytes part found
    return std::make_optional<FilePart>(fh, std::move(pp), overlayDisk.total_bytes, overlayDisk.current_bytes,
                                        overlayDisk.laste_time);
  }

private:
  HANDLE fileHandle{INVALID_HANDLE_VALUE};
  std::wstring path;
  int64_t total_bytes{0};
  int64_t current_bytes{0};
  int64_t laste_time{0};
  bool discard_file_handle{true};
  void file_discard() noexcept {
    if (fileHandle != INVALID_HANDLE_VALUE) {
      if (discard_file_handle) {
        disposition_file_handle(fileHandle);
      }
      CloseHandle(fileHandle);
    }
  }
};

} // namespace baulk::net::net_internal