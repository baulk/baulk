///
#include <charconv>
#include "tarinternal.hpp"

namespace baulk::archive::tar {
inline bool IsChecksumEqual(const ustar_header &hdr) {
  auto p = reinterpret_cast<const uint8_t *>(&hdr);
  auto value = parseNumeric(hdr.chksum);
  int64_t check = 0;
  int64_t scheck = 0;
  int i;
  for (i = 0; i < 148; i++) {
    check += p[i];
    scheck = static_cast<int8_t>(p[i]);
  }
  check += 256;
  scheck += 256;
  i = 156;
  for (; i < 512; i++) {
    check += p[i];
    scheck = static_cast<int8_t>(p[i]);
  }
  return (check == value || scheck == value);
}

tar_format_t getFormat(const ustar_header &hdr) {
  if (!IsChecksumEqual(hdr)) {
    return FormatUnknown;
  }
  auto star = reinterpret_cast<const star_header *>(&hdr);
  if (memcmp(hdr.magic, magicUSTAR, sizeof(hdr.magic)) == 0) {
    if (memcmp(star->trailer, trailerSTAR, sizeof(trailerSTAR)) == 0) {
      return FormatSTAR;
    }
    return FormatUSTAR | FormatPAX;
  }
  if (memcmp(hdr.magic, magicGNU, sizeof(magicGNU)) == 0 && memcmp(hdr.version, versionGNU, sizeof(versionGNU)) == 0) {
    return FormatGNU;
  }
  return FormatV7;
}
/*
 * Parse a base-256 integer.  This is just a variable-length
 * twos-complement signed binary value in big-endian order, except
 * that the high-order bit is ignored.  The values here can be up to
 * 12 bytes, so we need to be careful about overflowing 64-bit
 * (8-byte) integers.
 *
 * This code unashamedly assumes that the local machine uses 8-bit
 * bytes and twos-complement arithmetic.
 */
int64_t parseNumeric256(const char *_p, size_t char_cnt) {
  uint64_t l;
  auto *p = reinterpret_cast<const unsigned char *>(_p);
  unsigned char c{0}, neg{0};

  /* Extend 7-bit 2s-comp to 8-bit 2s-comp, decide sign. */
  c = *p;
  if (c & 0x40) {
    neg = 0xff;
    c |= 0x80;
    l = ~0ull;
  } else {
    neg = 0;
    c &= 0x7f;
    l = 0;
  }

  /* If more than 8 bytes, check that we can ignore
   * high-order bits without overflow. */
  while (char_cnt > sizeof(int64_t)) {
    --char_cnt;
    if (c != neg)
      return neg ? INT64_MIN : INT64_MAX;
    c = *++p;
  }

  /* c is first byte that fits; if sign mismatch, return overflow */
  if ((c ^ neg) & 0x80) {
    return neg ? INT64_MIN : INT64_MAX;
  }

  /* Accumulate remaining bytes. */
  while (--char_cnt > 0) {
    l = (l << 8) | c;
    c = *++p;
  }
  l = (l << 8) | c;
  /* Return signed twos-complement value. */
  return (int64_t)(l);
}

bool parsePAXTime(std::string_view p, bela::Time &t, bela::error_code &ec) {
  auto ss = p;
  std::string_view sn;
  if (auto pos = p.find('.'); pos != std::string_view::npos) {
    ss = p.substr(0, pos);
    sn = p.substr(pos + 1);
  }
  int64_t sec = 0;
  auto res = std::from_chars(ss.data(), ss.data() + ss.size(), sec);
  if (res.ec != std::errc{}) {
    ec = bela::make_error_code(bela::ErrGeneral, L"unable parse number '", bela::encode_into<char, wchar_t>(ss), L"'");
    return false;
  }
  if (sn.empty()) {
    t = bela::FromUnix(sec, 0);
    return true;
  }
  /* Calculate nanoseconds. */
  int64_t nsec{0};
  std::from_chars(ss.data(), ss.data() + ss.size(), sec);
  t = bela::FromUnix(sec, nsec);
  return true;
}

bool mergePAX(Header &h, pax_records_t &paxHdrs, bela::error_code &ec) {
  for (const auto &[k, v] : paxHdrs) {
    if (v.empty()) {
      continue;
    }
    if (k == paxPath) {
      h.Name = v;
      continue;
    }
    if (k == paxLinkpath) {
      h.LinkName = v;
      continue;
    }
    if (k == paxUname) {
      h.Uname = v;
      continue;
    }
    if (k == paxGname) {
      h.Gname = v;
      continue;
    }
    if (k == paxUid) {
      int64_t id{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), id);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse uid number '", bela::encode_into<char, wchar_t>(v),
                                   L"'");
        return false;
      }
      h.UID = static_cast<int>(id);
      continue;
    }
    if (k == paxGid) {
      int64_t id{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), id);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse gid number '", bela::encode_into<char, wchar_t>(v),
                                   L"'");
        return false;
      }
      h.GID = static_cast<int>(id);
      continue;
    }
    if (k == paxAtime) {
      if (parsePAXTime(v, h.AccessTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxMtime) {
      if (parsePAXTime(v, h.ModTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxCtime) {
      if (parsePAXTime(v, h.ChangeTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxSize) {
      int64_t size{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), size);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse size number '", bela::encode_into<char, wchar_t>(v),
                                   L"'");
        return false;
      }
      h.Size = size;
      continue;
    }
    if (k.starts_with(paxSchilyXattr)) {
      h.Xattrs.emplace(k.substr(paxSchilyXattr.size()), v);
    }
  }
  h.PAXRecords = std::move(paxHdrs);
  return true;
}
bool validPAXRecord(std::string_view k, std::string_view v) {
  if (k.empty() || k.find('=') != std::string_view::npos) {
    return false;
  }
  if (k == paxPath || k == paxLinkpath || k == paxUname || k == paxGname) {
    return v.find('\0') == std::string_view::npos;
  }
  return k.find('\0') == std::string_view::npos;
}

// %d %s=%s\n
bool parsePAXRecord(std::string_view *sv, std::string_view *k, std::string_view *v, bela::error_code &ec) {
  auto pos = sv->find(' ');
  if (pos == std::string_view::npos) {
    ec = bela::make_error_code(L"invalid pax record");
    return false;
  }
  int n = 0;
  if (auto res = std::from_chars(sv->data(), sv->data() + pos, n); res.ec != std::errc{} || n > sv->size()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"invalid number '",
                               bela::encode_into<char, wchar_t>(sv->substr(0, pos)), L"'");
    return false;
  }
  if (pos >= n) {
    ec = bela::make_error_code(L"invalid pax record");
    return false;
  }
  auto rec = sv->substr(pos + 1, n - pos - 1);
  if (!rec.ends_with('\n')) {
    ec = bela::make_error_code(L"invalid pax record");
    return false;
  }
  rec.remove_suffix(1);
  pos = rec.find('=');
  if (pos == std::string_view::npos) {
    ec = bela::make_error_code(L"invalid pax record");
    return false;
  }
  *k = rec.substr(0, pos);
  *v = rec.substr(pos + 1);
  sv->remove_prefix(n);
  if (!validPAXRecord(*k, *v)) {
    ec = bela::make_error_code(L"invalid tar pax record");
    return false;
  }
  return true;
}

bool validateSparseEntries(sparseDatas &spd, int64_t size) {
  if (size < 0) {
    return false;
  }
  sparseEntry pre;
  for (const auto &cur : spd) {
    if (cur.Offset < 0 || cur.Length < 0) {
      return false;
    }
    if (cur.Offset > (std::numeric_limits<int64_t>::max)() - cur.Length) {
      return false;
    }
    if (cur.endOffset() > size) {
      return false;
    }
    if (pre.endOffset() > cur.Offset) {
      return false;
    }
    pre = cur;
  }
  return true;
}

} // namespace baulk::archive::tar