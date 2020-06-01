// Escape Argv
#ifndef BELA_ESCAPE_ARGV_HPP
#define BELA_ESCAPE_ARGV_HPP
#include <string_view>
#include <vector>
#include "span.hpp"

namespace bela {

namespace argv_internal {
template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view Empty = "\"\"";
};
template <> class Literal<wchar_t> {
public:
#if defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  static constexpr std::wstring_view Empty = L"\"\"";
#else
  // libstdc++ call wcslen is bad
  static constexpr std::wstring_view Empty{L"\"\"", sizeof(L"\"\"") - sizeof(wchar_t)};
#endif
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view Empty = u"\"\"";
};

} // namespace argv_internal

// basic escape argv
template <typename charT, typename Allocator = std::allocator<charT>> class basic_escape_argv {
public:
  typedef std::basic_string_view<charT> string_view_t;
  typedef std::basic_string<charT, std::char_traits<charT>, Allocator> string_t;

  template <typename... Args> basic_escape_argv(string_view_t arg0, Args... arg) {
    string_view_t svv[] = {arg0, arg...};
    AssignFull(svv);
  }
  basic_escape_argv() = default;
  basic_escape_argv(const basic_escape_argv &) = delete;
  basic_escape_argv &operator=(const basic_escape_argv &) = delete;
  // AssignFull
  basic_escape_argv &AssignFull(const bela::Span<string_view_t> args) {
    struct arg_status {
      unsigned len{0};
      bool hasspace{false};
    };
    size_t totalsize = 0;
    std::vector<arg_status> avs(args.size());
    for (size_t i = 0; i < args.size(); i++) {
      auto ac = args[i];
      if (ac.empty()) {
        avs[i].len = 2; // "\"\""
        totalsize += 2;
        continue;
      }
      auto n = static_cast<unsigned>(ac.size());
      for (auto c : ac) {
        switch (c) {
        case L'"':
          [[fallthrough]];
        case L'\\':
          n++;
          break;
        case ' ':
          [[fallthrough]];
        case '\t':
          avs[i].hasspace = true;
          break;
        default:
          break;
        }
      }
      if (avs[i].hasspace) {
        n += 2;
      }
      avs[i].len = n;
      totalsize += n;
    }
    saver.reserve(args.size() + totalsize);
    for (size_t i = 0; i < args.size(); i++) {
      auto ac = args[i];
      if (!saver.empty()) {
        saver += ' ';
      }
      if (ac.empty()) {
        saver += argv_internal::Literal<charT>::Empty;
        continue;
      }
      if (ac.size() == avs[i].len) {
        saver.append(ac);
        continue;
      }
      if (avs[i].hasspace) {
        saver += '"';
      }
      size_t slashes = 0;
      for (auto c : ac) {
        switch (c) {
        case '\\':
          slashes++;
          saver += '\\';
          break;
        case '"': {
          for (; slashes > 0; slashes--) {
            saver += '\\';
          }
          saver += '\\';
          saver += c;
        } break;
        default:
          slashes = 0;
          saver += c;
          break;
        }
      }
      if (avs[i].hasspace) {
        for (; slashes > 0; slashes--) {
          saver += '\\';
        }
        saver += '"';
      }
    }
    return *this;
  }
  basic_escape_argv &AssignNoEscape(string_view_t a0) {
    saver.assign(a0);
    return *this;
  }
  basic_escape_argv &Assign(string_view_t arg0) {
    saver.clear();
    argv_escape_internal(arg0, saver);
    return *this;
  }
  basic_escape_argv &AppendNoEscape(string_view_t aN) {
    if (!saver.empty()) {
      saver += ' ';
    }
    saver.append(aN);
    return *this;
  }
  basic_escape_argv &Append(string_view_t aN) {
    argv_escape_internal(aN, saver);
    return *this;
  }
  const charT *data() const { return saver.data(); }
  charT *data() { return saver.data(); }
  string_view_t sv() const { return saver; }
  size_t size() const { return saver.size(); }

private:
  void argv_escape_internal(string_view_t sv, string_t &s) {
    if (!s.empty()) {
      s += ' ';
    }
    if (sv.empty()) {
      s += argv_internal::Literal<charT>::Empty;
      return;
    }
    bool hasspace = false;
    auto n = sv.size();
    for (auto c : sv) {
      switch (c) {
      case '"':
        [[fallthrough]];
      case '\\':
        n++;
        break;
      case ' ':
        [[fallthrough]];
      case '\t':
        hasspace = true;
        break;
      default:
        break;
      }
    }
    if (hasspace) {
      n += 2;
    }
    if (n == sv.size()) {
      s += sv;
      return;
    }
    s.reserve(s.size() + sv.size() + 1);
    if (hasspace) {
      s += '"';
    }
    size_t slashes = 0;
    for (auto c : sv) {
      switch (c) {
      case '\\':
        slashes++;
        s += '\\';
        break;
      case L'"': {
        for (; slashes > 0; slashes--) {
          s += '\\';
        }
        s += '\\';
        s += c;
      } break;
      default:
        slashes = 0;
        s += c;
        break;
      }
    }
    if (hasspace) {
      for (; slashes > 0; slashes--) {
        s += '\\';
      }
      s += '"';
    }
  }

  string_t saver;
};

using EscapeArgv = basic_escape_argv<wchar_t>;
} // namespace bela

#endif
