//////////
#ifndef CLANGBUILDER_TEMPLATE_STRING_HPP
#define CLANGBUILDER_TEMPLATE_STRING_HPP
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstddef>

inline constexpr bool IsWhitespace(wchar_t ch) {
  return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

inline constexpr bool IsRequireEscape(wchar_t ch) {
  return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' ||
         ch == L'\v' || ch == L'\"';
}

inline constexpr size_t StringLength(const wchar_t *s) {
  const wchar_t *a = s;
  for (; *a != 0; a++) {
    ;
  }
  return a - s;
}

inline constexpr bool StringEqual(const wchar_t *a, const wchar_t *b,
                                  size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

inline wchar_t *StringAllocate(size_t count) {
  return reinterpret_cast<wchar_t *>(
      HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t) * count));
}

inline void StringFree(wchar_t *p) {
  //
  HeapFree(GetProcessHeap(), 0, p);
}

inline void StringZero(wchar_t *p, size_t len) {
  for (size_t i = 0; i < len; i++) {
    p[i] = 0;
  }
}

inline void *StringCopy(wchar_t *dest, const wchar_t *src, size_t n) {
  auto d = dest;
  for (; n; n--) {
    *d++ = *src++;
  }
  return dest;
}

class StringView {
public:
  typedef wchar_t *pointer;
  typedef const wchar_t *const_pointer;
  typedef wchar_t &reference;
  typedef const wchar_t &const_reference;
  typedef const wchar_t *const_iterator;
  typedef size_t size_type;
  static constexpr size_type npos = size_type(-1);
  StringView() noexcept : data_(nullptr), size_(0) {}
  constexpr StringView(const wchar_t *str) noexcept
      : data_(str), size_(str == nullptr ? 0 : StringLength(str)) {}
  constexpr StringView(const wchar_t *str, size_type len) noexcept
      : data_(str), size_(len) {}
  constexpr const_iterator begin() const noexcept { return data_; }
  constexpr auto end() const noexcept { return data_ + size_; }
  constexpr auto size() const noexcept { return size_; }
  constexpr auto length() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr const_reference operator[](size_type i) const { return data_[i]; }
  constexpr const_pointer data() const noexcept { return data_; }

  constexpr StringView substr(size_type pos = 0, size_type n = npos) const {
    if (pos > size_) {
      pos = size_;
    }
    if (n > size_ - pos) {
      n = size_ - pos;
    }
    return StringView(data_ + pos, n);
  }

  constexpr const_reference front() const { return data_[0]; }
  constexpr const_reference back() const { return data_[size_ - 1]; }

  void remove_prefix(size_type n) {
    data_ += n;
    size_ -= n;
  }
  void remove_suffix(size_type n) { size_ -= n; }

  constexpr bool starts_with(const StringView &x) const noexcept {
    return x.empty() ||
           (size() >= x.size() && StringEqual(data(), x.data(), x.size()));
  }

  // Does "this" end with "x"?
  constexpr bool ends_with(const StringView &x) const noexcept {
    return x.empty() ||
           (size() >= x.size() &&
            StringEqual(data() + (size() - x.size()), x.data(), x.size()));
  }
  constexpr size_type find(wchar_t c, size_t pos) const noexcept {
    if (size_ == 0 || pos >= size_) {
      return npos;
    }
    for (size_t index = pos; index < size_; index++) {
      if (c == data_[index]) {
        return index;
      }
    }
    return npos;
  }

private:
  const_pointer data_;
  size_type size_;
};

inline constexpr bool operator==(const StringView &x, const StringView &y) {
  StringView::size_type len = x.size();
  if (len != y.size()) {
    return false;
  }
  return x.data() == y.data() || len == 0 ||
         StringEqual(x.data(), y.data(), len);
}

inline constexpr bool operator!=(const StringView &x, const StringView &y) {
  return !(x == y);
}

inline constexpr bool IsEscapeArg0(StringView str) {
  for (auto c : str) {
    if (IsRequireEscape(c)) {
      return true;
    }
  }
  return false;
}

inline constexpr StringView StripTrailingWhitespace(StringView str) {
  for (size_t i = 0; i < str.size(); i++) {
    if (!IsWhitespace(str[i])) {
      return str.substr(i, str.size() - i);
    }
  }
  return StringView();
}

inline constexpr StringView StripArg0(StringView cmdline) {
  cmdline = StripTrailingWhitespace(cmdline);
  if (cmdline.empty()) {
    return StringView();
  }
  if (cmdline.front() != L'"') {
    auto pos = cmdline.find(L' ', 0);
    if (pos == StringView::npos) {
      return StringView();
    }
    return cmdline.substr(pos + 1);
  }
  return StringView();
}

inline bool EscapeArgv(StringView cmdline, StringView target, wchar_t **saver) {
  if (saver == nullptr) {
    return false;
  }
  auto args = StripArg0(cmdline);
  size_t n = args.size() + target.size() + 4; // "" args
  auto p = StringAllocate(n);
  if (p == nullptr) {
    return false;
  }
  StringZero(p, n);
  auto it = p;
  if (IsEscapeArg0(target)) {
    *p++ = L'"';
    StringCopy(p, target.data(), target.size());
    p += target.size();
    *p++ = L'"';
  } else {
    StringCopy(p, target.data(), target.size());
    p += target.size();
  }
  if (!args.empty()) {
    *p++ = L' ';
    StringCopy(p, args.data(), args.size());
  }
  *saver = p;
  return true;
}

#endif