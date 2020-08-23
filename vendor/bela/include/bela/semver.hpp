// Thanks Neargye/semver
// origin code see
// https://github.com/Neargye/semver/blob/master/include/semver.hpp
// support wchar_t
#ifndef BELA_SENVER_HPP
#define BELA_SENVER_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace bela::semver {
enum struct prerelease : std::uint8_t {
  alpha = 0, // alpha
  beta = 1,
  rc = 2,
  none = 3
};

// Max version string length = 3(<major>) + 1(.) + 3(<minor>) + 1(.) +
// 3(<patch>) + 1(-) + 5(<prerelease>) + 1(.) + 3(<prereleaseversion>) = 21.
[[maybe_unused]] inline constexpr std::size_t max_version_string_length = 21;

namespace detail {
// Literal
template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view Alpha{"-alpha"};
  static constexpr std::string_view Beta{"-beta"};
  static constexpr std::string_view RC{"-rc"};
  static constexpr char Delimiter{'.'};
};
template <> class Literal<wchar_t> {
public:
  static constexpr std::wstring_view Alpha{L"-alpha"};
  static constexpr std::wstring_view Beta{L"-beta"};
  static constexpr std::wstring_view RC{L"-rc"};
  static constexpr wchar_t Delimiter{L'.'};
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view Alpha{u"-alpha"};
  static constexpr std::u16string_view Beta{u"-beta"};
  static constexpr std::u16string_view RC{u"-rc"};
  static constexpr char16_t Delimiter{u'.'};
};
//
template <typename CharT> struct from_chars_result {
  const CharT *ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};
template <typename CharT> struct to_chars_result {
  CharT *ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};

// Min version string length = 1(<major>) + 1(.) + 1(<minor>) + 1(.) +
// 1(<patch>) = 5.
inline constexpr auto min_version_string_length = 5;

template <typename CharT> constexpr CharT to_lower(CharT c) noexcept {
  return (c >= 'A' && c <= 'Z') ? static_cast<CharT>(c + ('a' - 'A')) : c;
}

template <typename CharT> constexpr bool is_digit(CharT c) noexcept { return c >= '0' && c <= '9'; }
template <typename CharT> constexpr std::uint8_t to_digit(CharT c) noexcept {
  return static_cast<std::uint8_t>(c - '0');
}
constexpr std::uint8_t length(std::uint8_t x) noexcept { return x < 10 ? 1 : (x < 100 ? 2 : 3); }

constexpr std::uint8_t length(prerelease t) noexcept {
  if (t == prerelease::alpha) {
    return 5;
  }
  if (t == prerelease::beta) {
    return 4;
  }
  if (t == prerelease::rc) {
    return 2;
  }
  return 0;
}

template <typename CharT>
constexpr bool equals(const CharT *first, const CharT *last, std::basic_string_view<CharT> str) noexcept {
  for (std::size_t i = 0; first != last && i < str.size(); ++i, ++first) {
    if (to_lower(*first) != to_lower(str[i])) {
      return false;
    }
  }
  return true;
}

template <typename CharT> constexpr CharT *to_chars(CharT *str, std::uint8_t x, bool dot = true) noexcept {
  do {
    *(--str) = static_cast<CharT>('0' + (x % 10));
    x /= 10;
  } while (x != 0);

  if (dot) {
    *(--str) = '.';
  }
  return str;
}

template <typename CharT> constexpr CharT *to_chars(CharT *str, prerelease t) noexcept {
  const auto p = t == prerelease::alpha ? Literal<CharT>::Alpha
                                        : t == prerelease::beta ? Literal<CharT>::Beta
                                                                : t == prerelease::rc ? Literal<CharT>::RC
                                                                                      : std::basic_string_view<CharT>{};
  for (auto it = p.rbegin(); it != p.rend(); ++it) {
    *(--str) = *it;
  }
  return str;
}

template <typename CharT>
constexpr const CharT *from_chars(const CharT *first, const CharT *last, std::uint8_t &d) noexcept {
  if (first != last && is_digit(*first)) {
    std::int32_t t = 0;
    for (; first != last && is_digit(*first); ++first) {
      t = t * 10 + to_digit(*first);
    }
    if (t <= (std::numeric_limits<std::uint8_t>::max)()) {
      d = static_cast<std::uint8_t>(t);
      return first;
    }
  }
  return nullptr;
}

template <typename CharT>
constexpr const CharT *from_chars(const CharT *first, const CharT *last, prerelease &p) noexcept {
  if (equals(first, last, Literal<CharT>::Alpha)) {
    p = prerelease::alpha;
    return first + Literal<CharT>::Alpha.size();
  }
  if (equals(first, last, Literal<CharT>::Beta)) {
    p = prerelease::beta;
    return first + Literal<CharT>::Beta.size();
  }
  if (equals(first, last, Literal<CharT>::RC)) {
    p = prerelease::rc;
    return first + Literal<CharT>::RC.size();
  }
  return nullptr;
}

template <typename CharT> constexpr bool check_delimiter(const CharT *first, const CharT *last, CharT d) noexcept {
  return first != last && first != nullptr && *first == d;
}
} // namespace detail

struct version {
  std::uint8_t major = 0;
  std::uint8_t minor = 1;
  std::uint8_t patch = 0;
  prerelease prerelease_type = prerelease::none;
  std::uint8_t prerelease_number = 0;

  constexpr version(std::uint8_t major, std::uint8_t minor, std::uint8_t patch,
                    prerelease prerelease_type = prerelease::none, std::uint8_t prerelease_number = 0) noexcept
      : major{major}, minor{minor}, patch{patch}, prerelease_type{prerelease_type},
        prerelease_number{prerelease_type == prerelease::none ? static_cast<std::uint8_t>(0) : prerelease_number} {}

  constexpr version(std::string_view str) : version(0, 0, 0, prerelease::none, 0) { from_string_noexcept(str); }

  constexpr version(std::wstring_view str) : version(0, 0, 0, prerelease::none, 0) { from_string_noexcept(str); }

  constexpr version() = default;
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
  constexpr version(const version &) = default;
  constexpr version(version &&) = default;
  ~version() = default;
  constexpr version &operator=(const version &) = default;
  constexpr version &operator=(version &&) = default;

  template <typename CharT>
  [[nodiscard]] constexpr detail::to_chars_result<CharT> to_chars(CharT *first, CharT *last) const noexcept {
    const auto length = chars_length();
    if (first == nullptr || last == nullptr || (last - first) < length) {
      return {last, std::errc::value_too_large};
    }
    auto next = first + length;
    if (prerelease_type != prerelease::none) {
      if (prerelease_number != 0) {
        next = detail::to_chars(next, prerelease_number);
      }
      next = detail::to_chars(next, prerelease_type);
    }
    next = detail::to_chars(next, patch);
    next = detail::to_chars(next, minor);
    next = detail::to_chars(next, major, false);
    return {first + length, std::errc{}};
  }

  [[nodiscard]] std::wstring to_wstring() const {
    auto str = std::wstring(chars_length(), '\0');
    if (!to_chars(str.data(), str.data() + str.length())) {
      return L"";
    }
    return str;
  }

  [[nodiscard]] std::string to_string() const {
    auto str = std::string(chars_length(), '\0');
    if (!to_chars(str.data(), str.data() + str.length())) {
      return "";
    }
    return str;
  }

  template <typename CharT>
  [[nodiscard]] constexpr detail::from_chars_result<CharT> from_chars(const CharT *first, const CharT *last) noexcept {
    if (first == nullptr || last == nullptr || (last - first) < detail::min_version_string_length) {
      return {first, std::errc::invalid_argument};
    }
    auto next = first;
    if (next = detail::from_chars(next, last, major); detail::check_delimiter(next, last, static_cast<CharT>('.'))) {
      if (next = detail::from_chars(++next, last, minor);
          detail::check_delimiter(next, last, static_cast<CharT>('.'))) {
        if (next = detail::from_chars(++next, last, patch); next == last) {
          prerelease_type = prerelease::none;
          prerelease_number = 0;
          return {next, std::errc{}};
        }
        if (detail::check_delimiter(next, last, static_cast<CharT>('-'))) {
          if (next = detail::from_chars(next, last, prerelease_type); next == last) {
            prerelease_number = 0;
            return {next, std::errc{}};
          }
          if (detail::check_delimiter(next, last, static_cast<CharT>('.'))) {
            if (next = detail::from_chars(++next, last, prerelease_number); next == last) {
              return {next, std::errc{}};
            }
          }
        }
      }
    }
    return {first, std::errc::invalid_argument};
  }

  template <typename T> constexpr bool from_string_noexcept_t(std::basic_string_view<T> str) noexcept {
    return from_chars(str.data(), str.data() + str.size());
  }

  constexpr bool from_string_noexcept(std::string_view str) noexcept { return from_string_noexcept_t(str); }

  constexpr bool from_string_noexcept(std::wstring_view str) noexcept { return from_string_noexcept_t(str); }

  [[nodiscard]] constexpr int compare(const version &other) const noexcept {
    if (major != other.major) {
      return major - other.major;
    }
    if (minor != other.minor) {
      return minor - other.minor;
    }
    if (patch != other.patch) {
      return patch - other.patch;
    }
    if (prerelease_type != other.prerelease_type) {
      return static_cast<std::uint8_t>(prerelease_type) - static_cast<std::uint8_t>(other.prerelease_type);
    }
    if (prerelease_number != other.prerelease_number) {
      return prerelease_number - other.prerelease_number;
    }
    return 0;
  }

private:
  constexpr std::uint8_t chars_length() const noexcept {
    // (<major>) + 1(.) + (<minor>) + 1(.) + (<patch>)
    std::uint8_t length = detail::length(major) + detail::length(minor) + detail::length(patch) + 2;
    if (prerelease_type != prerelease::none) {
      // + 1(-) + (<prerelease>)
      length += detail::length(prerelease_type) + 1;
      if (prerelease_number != 0) {
        // + 1(.) + (<prereleaseversion>)
        length += detail::length(prerelease_number) + 1;
      }
    }
    return length;
  }
};

constexpr bool operator==(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) == 0; }

constexpr bool operator!=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) != 0; }

constexpr bool operator>(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) > 0; }

constexpr bool operator>=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) >= 0; }

constexpr bool operator<(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) < 0; }

constexpr bool operator<=(const version &lhs, const version &rhs) noexcept { return lhs.compare(rhs) <= 0; }

constexpr version operator""_version(const char *str, std::size_t size) { return version{std::string_view{str, size}}; }

inline std::string to_string(const version &v) { return v.to_string(); }
inline std::wstring to_wstring(const version &v) { return v.to_wstring(); }

constexpr std::optional<version> from_string_noexcept(std::string_view str) noexcept {
  if (version v{0, 0, 0}; v.from_string_noexcept_t(str)) {
    return v;
  }
  return std::nullopt;
}
constexpr std::optional<version> from_string_noexcept(std::wstring_view str) noexcept {
  if (version v{0, 0, 0}; v.from_string_noexcept_t(str)) {
    return v;
  }
  return std::nullopt;
}

} // namespace bela::semver

#endif
