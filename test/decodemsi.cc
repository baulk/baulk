#include <bela/terminal.hpp>
#include <bela/numbers.hpp>

struct Helper {
  int field[4];
  bool Decode(std::wstring_view msg);
};

namespace baulk {
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, const Args &...args) {
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}

} // namespace baulk
int FGetInteger(const wchar_t *&it, const wchar_t *end) {
  auto pchPrev = it;
  while (it < end && *it != ' ') {
    it++;
  }
  std::wstring_view sv{pchPrev, static_cast<size_t>(it - pchPrev)};
  int i = 0;
  (void)bela::SimpleAtoi(sv, &i);
  return i;
}

bool Helper::Decode(std::wstring_view msg) {
  if (msg.empty()) {
    return false;
  }
  baulk::DbgPrint(L"%s", msg);
  auto it = msg.data();
  auto end = it + msg.size();
  while (it < end) {
    auto ch = *it;
    it += 3; // skip ': '
    if (it >= end) {
      return false;
    }
    baulk::DbgPrint(L">%s | %c", msg, ch);
    switch (ch) {
    case '1': {
      auto ch_ = *it;
      it++;
      baulk::DbgPrint(L"*%s | %c", msg, ch_);
      if (isdigit(ch_) == 0) {
        return false;
      }
      field[0] = ch_ - '0';
      baulk::DbgPrint(L">%s | %c", msg, ch_);
    } break;
    case '2':
      field[1] = FGetInteger(it, end);
      if (field[0] == 2 || field[0] == 3) {
        return true;
      }
      break;
    case 3:
      field[2] = FGetInteger(it, end);
      if (field[0] == 1) {
        return true;
      }
    case '4':
      field[3] = FGetInteger(it, end);
      return true;
    default:
      return false;
    }
    it++;
  }
  return true;
}

int main() {
  constexpr std::wstring_view msgs[] = {L"1: 2 2: 4854 3: 0 4: 0", L"1: 2 2: 124324 3: 0 4: 0", L"1: 0 2: 0 3: 0 4: 0"};
  Helper h;
  memset(h.field, 0, sizeof(h.field));
  for (auto s : msgs) {
    auto b = h.Decode(s);
    bela::FPrintF(stderr, L"%d-%d-%d-%d %b\n", h.field[0], h.field[1], h.field[2], h.field[3], b);
  }
  return 0;
}