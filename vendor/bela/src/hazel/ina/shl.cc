////////////
#include "shl.hpp"
#include "hazelinc.hpp"
#include <bela/endian.hpp>
/// shutcut resolve
// 4C 00 00 00 01 14 02 00 00 00 00 00 C0 00 00 00 00 00 00 46

namespace shl {
struct link_value_flags_t {
  uint32_t v;
  const wchar_t *n;
};

inline void FlagsToArray(uint32_t flag, std::vector<std::wstring> &av) {
  static const link_value_flags_t lfv[] = {
      {.v = HasLinkTargetIDList, .n = L"HasLinkTargetIDList"},
      {.v = HasLinkInfo, .n = L"HasLinkInfo"},
      {.v = HasName, .n = L"HasName"},
      {.v = HasRelativePath, .n = L"HasRelativePath"},
      {.v = HasWorkingDir, .n = L"HasWorkingDir"},
      {.v = HasArguments, .n = L"HasArguments"},
      {.v = HasIconLocation, .n = L"HasIconLocation"},
      {.v = IsUnicode, .n = L"IsUnicode"},
      {.v = ForceNoLinkInfo, .n = L"ForceNoLinkInfo"},
      {.v = HasExpString, .n = L"HasExpString"},
      {.v = RunInSeparateProcess, .n = L"RunInSeparateProcess"},
      {.v = HasDrawinID, .n = L"HasDrawinID"},
      {.v = RunAsUser, .n = L"RunAsUser"},
      {.v = HasExpIcon, .n = L"HasExpIcon"},
      {.v = NoPidlAlias, .n = L"NoPidlAlias"},
      {.v = RunWithShimLayer, .n = L"RunWithShimLayer"},
      {.v = ForceNoLinkTrack, .n = L"ForceNoLinkTrack"},
      {.v = EnableTargetMetadata, .n = L"EnableTargetMetadata"},
      {.v = DisableLinkPathTarcking, .n = L"DisableLinkPathTarcking"},
      {.v = DisableKnownFolderTarcking, .n = L"DisableKnownFolderTarcking"},
      {.v = DisableKnownFolderAlia, .n = L"DisableKnownFolderAlia"},
      {.v = AllowLinkToLink, .n = L"AllowLinkToLink"},
      {.v = UnaliasOnSave, .n = L"UnaliasOnSave"},
      {.v = PreferEnvironmentPath, .n = L"PreferEnvironmentPath"},
      {.v = KeepLocalIDListForUNCTarget, .n = L"KeepLocalIDListForUNCTarget"},
      {.v = PersistVolumeIDRelative, .n = L"PersistVolumeIDRelative"} //
  };
  for (const auto &v : lfv) {
    if ((v.v & flag) != 0) {
      av.emplace_back(v.n);
    }
  }
}
} // namespace shl

namespace hazel::internal {

static inline std::wstring shl_fromascii(std::string_view sv) {
  auto sz = MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), output.data(), sz);
  return output;
}

class shl_memview {
public:
  shl_memview(const char *data_, size_t size_) : data_(data_), size_(size_) {
    //
  }
  // --> perpare shl memview
  bool prepare() {
    constexpr uint8_t shuuid[] = {0x1, 0x14, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46};
    if (size_ < sizeof(shl::shell_link_t)) {
      return false;
    }
    auto dwSize = bela::cast_fromle<uint32_t>(data_);
    if (dwSize != 0x0000004C) {
      return false;
    }
    if (memcmp(data_ + 4, shuuid, sizeof(shuuid)) != 0) {
      return false;
    }
    linkflags_ = bela::cast_fromle<uint32_t>(data_ + 20);
    IsUnicode = (linkflags_ & shl::IsUnicode) != 0;
    return true;
  }

  [[nodiscard]] const char *data() const { return data_; }
  [[nodiscard]] size_t size() const { return size_; }

  template <typename T> [[nodiscard]] const T *cast(size_t off) const {
    if (off + sizeof(T) >= size_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(data_ + off);
  }

  [[nodiscard]] uint32_t linkflags() const { return linkflags_; }

  bool stringdata(size_t pos, std::wstring &sd, size_t &sdlen) const {
    if (pos + 2 > size_) {
      return false;
    }
    // CountCharacters (2 bytes): A 16-bit, unsigned integer that specifies
    // either the number of characters, defined by the system default code page,
    // or the number of Unicode characters found in the String field. A value of
    // zero specifies an empty string.
    // String (variable): An optional set of characters, defined by the system
    // default code page, or a Unicode string with a length specified by the
    // CountCharacters field. This string MUST NOT be NULL-terminated.

    auto len = bela::cast_fromle<uint16_t>(data_ + pos); /// Ch
    if (IsUnicode) {
      sdlen = len * 2 + 2;
      if (sdlen + pos >= size_) {
        return false;
      }
      auto *p = reinterpret_cast<const uint16_t *>(data_ + pos + 2);
      sd.clear();
      for (size_t i = 0; i < len; i++) {
        // Winodws UTF16LE
        sd.push_back(bela::fromle(p[i]));
      }
      return true;
    }

    sdlen = len + 2;
    if (sdlen + pos >= size_) {
      return false;
    }
    auto w = shl_fromascii(std::string_view(data_ + pos + 2, len));
    return true;
  }

  bool stringvalue(size_t pos, bool isu, std::wstring &su) {
    if (pos >= size_) {
      return false;
    }
    if (!isu) {
      auto begin = data_ + pos;
      auto end = data_ + size_;
      for (auto it = begin; it != end; it++) {
        if (*it == 0) {
          su = shl_fromascii(std::string_view(begin, it - begin));
          return true;
        }
      }
      return false;
    }
    auto it = (const wchar_t *)(data_ + pos);
    auto end = it + ((size_ - pos) / 2);
    for (; it != end; it++) {
      if (*it == 0) {
        return true;
      }
      su.push_back(bela::fromle(*it));
    }
    return false;
  }

private:
  const char *data_{nullptr};
  size_t size_{0};
  uint32_t linkflags_{0};
  bool IsUnicode{false};
};

// LINKINFO

// VolumeIDOffset (4 bytes): A 32-bit, unsigned integer that specifies the
// location of the VolumeID field. If the VolumeIDAndLocalBasePath flag is set,
// this value is an offset, in bytes, from the start of the LinkInfo structure;
// otherwise, this value MUST be zero.

// CommonNetworkRelativeLinkOffset (4 bytes): A 32-bit, unsigned integer that
// specifies the location of the CommonNetworkRelativeLink field. If the
// CommonNetworkRelativeLinkAndPathSuffix flag is set, this value is an offset,
// in bytes, from the start of the LinkInfo structure; otherwise, this value
// MUST be zero.

// LocalBasePathOffset (4 bytes): A 32-bit, unsigned integer that specifies
// the location of the LocalBasePath field. If the VolumeIDAndLocalBasePath
// flag is set, this value is an offset, in bytes, from the start of the
// LinkInfo structure; otherwise, this value MUST be zero.

// LocalBasePathOffsetUnicode (4 bytes): An optional, 32-bit, unsigned
// integer that specifies the location of the LocalBasePathUnicode field. If
// the VolumeIDAndLocalBasePath flag is set, this value is an offset, in
// bytes, from the start of the LinkInfo structure; otherwise, this value
// MUST be zero. This field can be present only if the value of the
// LinkInfoHeaderSize field is greater than or equal to 0x00000024.

// CommonPathSuffixOffsetUnicode (4 bytes): An optional, 32-bit, unsigned
// integer that specifies the location of the CommonPathSuffixUnicode field.
// This value is an offset, in bytes, from the start of the LinkInfo structure.
// This field can be present only if the value of the LinkInfoHeaderSize field
// is greater than or equal to 0x00000024.

// VolumeID (variable): An optional VolumeID structure (section 2.3.1) that
// specifies information about the volume that the link target was on when the
// link was created. This field is present if the VolumeIDAndLocalBasePath flag
// is set.

// LocalBasePath (variable): An optional, NULL–terminated string, defined by the
// system default code page, which is used to construct the full path to the
// link item or link target by appending the string in the CommonPathSuffix
// field. This field is present if the VolumeIDAndLocalBasePath flag is set.

// CommonNetworkRelativeLink (variable): An optional CommonNetworkRelativeLink
// structure (section 2.3.2) that specifies information about the network
// location where the link target is stored.

// CommonPathSuffix (variable): A NULL–terminated string, defined by the system
// default code page, which is used to construct the full path to the link item
// or link target by being appended to the string in the LocalBasePath field.

// LocalBasePathUnicode (variable): An optional, NULL–terminated, Unicode string
// that is used to construct the full path to the link item or link target by
// appending the string in the CommonPathSuffixUnicode field. This field can be
// present only if the VolumeIDAndLocalBasePath flag is set and the value of the
// LinkInfoHeaderSize field is greater than or equal to 0x00000024.

// CommonPathSuffixUnicode (variable): An optional, NULL–terminated, Unicode
// string that is used to construct the full path to the link item or link
// target by being appended to the string in the LocalBasePathUnicode field.
// This field can be present only if the value of the LinkInfoHeaderSize field
// is greater than or equal to 0x00000024

status_t LookupShellLink(const bela::bytes_view &bv, hazel_result &hr) {
  shl_memview shm(reinterpret_cast<const char *>(bv.data()), bv.size());
  if (!shm.prepare()) {
    return None;
  }
  auto flag = shm.linkflags();
  size_t offset = sizeof(shl::shell_link_t);
  if ((flag & shl::HasLinkTargetIDList) != 0) {
    if (shm.size() <= offset + 2) {
      return None;
    }
    auto l = bela::cast_fromle<uint16_t>(shm.data() + offset);
    if (l + 2 + offset >= shm.size()) {
      return None;
    }

    offset += l + 2;
  }

  hr.assign(types::lnk, L"Windows Shortcut");
  std::vector<std::wstring> av;
  shl::FlagsToArray(flag, av);
  hr.append(L"Attribute", std::move(av));

  // LinkINFO https://msdn.microsoft.com/en-us/library/dd871404.aspx
  if ((flag & shl::HasLinkInfo) != 0) {
    auto li = shm.cast<shl::shl_link_infow_t>(offset);
    if (li == nullptr) {
      return Found;
    }
    auto liflag = bela::fromle(li->dwFlags);
    if ((liflag & shl::VolumeIDAndLocalBasePath) != 0) {
      std::wstring su;
      bool isunicode;
      size_t pos;
      if (bela::fromle(li->cbHeaderSize) < 0x00000024) {
        isunicode = false;
        pos = offset + bela::fromle(li->cbLocalBasePathOffset);
      } else {
        isunicode = true;
        pos = offset + bela::fromle(li->cbLocalBasePathUnicodeOffset);
      }

      if (!shm.stringvalue(pos, isunicode, su)) {
        return Found;
      }
      hr.append(L"Target", su);
    } else if ((liflag & shl::CommonNetworkRelativeLinkAndPathSuffix) != 0) {
      //// NetworkRelative
    }
    offset += bela::fromle(li->cbSize);
  }
  // StringData https://msdn.microsoft.com/en-us/library/dd871306.aspx
  static const shl::link_value_flags_t sdv[] = {
      {.v = shl::HasName, .n = L"Name"},
      {.v = shl::HasRelativePath, .n = L"RelativePath"},
      {.v = shl::HasWorkingDir, .n = L"WorkingDir"},
      {.v = shl::HasArguments, .n = L"Arguments"},
      {.v = shl::HasIconLocation, .n = L"IconLocation"} /// --->
  };

  for (const auto &i : sdv) {
    std::wstring sd;
    size_t sdlen = 0;
    if ((flag & i.v) == 0) {
      continue;
    }
    if (!shm.stringdata(offset, sd, sdlen)) {
      return Found;
    }
    offset += sdlen;
    hr.append(i.n, sd);
  }

  // ExtraData https://msdn.microsoft.com/en-us/library/dd891345.aspx
  if ((flag & shl::EnableTargetMetadata) != 0) {
    //// such edge
  }

  return Found;
}
} // namespace hazel::internal