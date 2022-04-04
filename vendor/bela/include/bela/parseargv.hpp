///////
#ifndef BELA_PARSEARGV_HPP
#define BELA_PARSEARGV_HPP
#pragma once
#include <functional>
#include <vector>
#include "base.hpp"

namespace bela {
enum HasArgs {
  required_argument, /// -i 11 or -i=xx
  no_argument,
  optional_argument /// -s --long --long=xx
};

struct option {
  option() = default;
  option(std::wstring_view n, HasArgs ha, int va) : name(n), has_args(ha), val(va) {} /// emplace_back
  std::wstring_view name;
  HasArgs has_args;
  int val;
};

using invoke_t = std::function<bool(int, const wchar_t *, const wchar_t *)>;
// ParseArgv to resolve long or short argument
class ParseArgv {
public:
  using StringArray = std::vector<std::wstring_view>;
  ParseArgv(int argc, wchar_t *const *argv, bool subcmd_mode_ = false)
      : argc_(argc), argv_(argv), subcmd_mode(subcmd_mode_) {}
  ParseArgv(const ParseArgv &) = delete;
  ParseArgv &operator=(const ParseArgv &) = delete;
  ParseArgv &Add(std::wstring_view name, HasArgs a, int val) {
    options_.emplace_back(name, a, val);
    return *this;
  }
  bool Execute(const invoke_t &v, bela::error_code &ec);
  const StringArray &UnresolvedArgs() const { return uargs; }

private:
  const int argc_;
  const wchar_t *const *argv_;
  int index{0};
  bool subcmd_mode{false};
  StringArray uargs;
  std::vector<option> options_;
  bool parse_internal(std::wstring_view a, const invoke_t &v, bela::error_code &ec);
  bool parse_internal_long(std::wstring_view a, const invoke_t &v, bela::error_code &ec);
  bool parse_internal_short(std::wstring_view a, const invoke_t &v, bela::error_code &ec);
};

// ---------> parse internal
inline bool ParseArgv::Execute(const invoke_t &v, bela::error_code &ec) {
  if (argc_ == 0 || argv_ == nullptr) {
    ec = bela::make_error_code(ErrParseBroken, L"argv is empty.");
    return false;
  }
  constexpr std::wstring_view subcmd_parse_end_mark = L"--";
  index = 1;
  for (; index < argc_; index++) {
    std::wstring_view a = argv_[index];
    if (subcmd_mode && subcmd_parse_end_mark == a) {
      for (int i = index + 1; i < argc_; i++) {
        uargs.emplace_back(argv_[i]);
      }
      return true;
    }
    if (a.empty() || a.front() != '-') {
      // if subcmd mode. save all args
      if (subcmd_mode) {
        for (int i = index; i < argc_; i++) {
          uargs.emplace_back(argv_[i]);
        }
        return true;
      }
      uargs.emplace_back(a);
      continue;
    }
    // parse ---
    if (!parse_internal(a, v, ec)) {
      return false;
    }
  }
  return true;
}

inline bool ParseArgv::parse_internal_short(std::wstring_view a, const invoke_t &v, bela::error_code &ec) {
  int ch = -1;
  HasArgs ha = optional_argument;
  const wchar_t *oa = nullptr;
  // -x=XXX
  // -xXXX
  // -x XXX
  // -x; BOOL
  if (a[0] == '=') {
    ec = bela::make_error_code(ErrParseBroken, L"unexpected argument '-", a,
                               L"'"); // -=*
    return false;
  }
  auto c = a[0];
  for (const auto &o : options_) {
    if (o.val == c) {
      ch = o.val;
      ha = o.has_args;
      break;
    }
  }
  if (ch == -1) {
    ec = bela::make_error_code(ErrParseBroken, L"unregistered option '-", a, L"'");
    return false;
  }
  // a.size()==1 'L' short value
  if (a.size() >= 2) {
    oa = (a[1] == L'=') ? (a.data() + 2) : (a.data() + 1);
  }
  if (oa != nullptr && ha == no_argument) {
    ec = bela::make_error_code(ErrParseBroken, L"option '-", a.substr(0, 1), L"' unexpected parameter: ", oa);
    return false;
  }
  if (oa == nullptr && ha == required_argument) {
    if (index + 1 >= argc_) {
      ec = bela::make_error_code(ErrParseBroken, L"option '-", a, L"' missing parameter");
      return false;
    }
    oa = argv_[index + 1];
    index++;
  }
  if (!v(ch, oa, a.data())) {
    ec = bela::make_error_code(ErrSkipParse, L"skip parse");
    return false;
  }
  return true;
}

// Parse long option
inline bool ParseArgv::parse_internal_long(std::wstring_view a, const invoke_t &v, bela::error_code &ec) {
  // --xxx=XXX
  // --xxx XXX
  // --xxx; bool
  int ch = -1;
  HasArgs ha = optional_argument;
  const wchar_t *oa = nullptr;
  auto pos = a.find(L'=');
  if (pos != std::string_view::npos) {
    if (pos + 1 >= a.size()) {
      ec = bela::make_error_code(ErrParseBroken, L"unexpected argument '--", a, L"'");
      return false;
    }
    oa = a.data() + pos + 1;
    a.remove_suffix(a.size() - pos);
  }
  for (const auto &o : options_) {
    if (o.name == a) {
      ch = o.val;
      ha = o.has_args;
      break;
    }
  }
  if (ch == -1) {
    ec = bela::make_error_code(ErrParseBroken, L"unregistered option '--", a, L"'");
    return false;
  }
  if (oa != nullptr && ha == no_argument) {
    ec = bela::make_error_code(ErrParseBroken, L"option '--", a, L"' unexpected parameter: ", oa);
    return false;
  }
  if (oa == nullptr && ha == required_argument) {
    if (index + 1 >= argc_) {
      ec = bela::make_error_code(ErrParseBroken, L"option '--", a, L"' missing parameter");
      return false;
    }
    oa = argv_[index + 1];
    index++;
  }
  if (!v(ch, oa, a.data())) {
    ec = bela::make_error_code(ErrSkipParse, L"skip parse");
    return false;
  }
  return true;
}

inline bool ParseArgv::parse_internal(std::wstring_view a, const invoke_t &v, bela::error_code &ec) {
  if (a.size() == 1) {
    ec = bela::make_error_code(ErrParseBroken, L"unexpected argument '-'");
    return false;
  }
  if (a[1] == '-') {
    return parse_internal_long(a.substr(2), v, ec);
  }
  return parse_internal_short(a.substr(1), v, ec);
}

} // namespace bela

#endif
