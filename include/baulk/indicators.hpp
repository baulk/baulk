/// Learn from https://github.com/p-ranav/indicators
#ifndef BAULK_INDICATORS_HPP
#define BAULK_INDICATORS_HPP
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <thread>
#include <condition_variable>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <bela/terminal.hpp>

namespace baulk {
inline bool has_overbyte(std::wstring_view sv) {
  for (const auto c : sv) {
    if (c > 0x7f) {
      return true;
    }
  }
  return false;
}

struct character {
  char32_t rune;
  size_t width;
};
using characters = std::vector<character>;

inline void encode_into_unicode(std::wstring_view sv, characters &us, size_t &usLen) {
  size_t width = 0;
  auto it = sv.data();
  auto end = it + sv.size();
  us.reserve(sv.size());
  us.clear();
  while (it < end) {
    char32_t rune = *it++;
    if (rune >= 0xD800 && rune <= 0xDBFF) {
      if (it >= end) {
        break;
      }
      char32_t rune2 = *it;
      if (rune2 < 0xDC00 || rune2 > 0xDFFF) {
        break;
      }
      rune = ((rune - 0xD800) << 10) + (rune2 - 0xDC00) + 0x10000U;
      ++it;
    }
    auto width = bela::rune_width(rune);
    usLen += width;
    us.emplace_back(character{
        .rune = rune,
        .width = width,
    });
  }
}
constexpr bool rune_is_surrogate(char32_t rune) { return (rune >= 0xD800 && rune <= 0xDFFF); }

inline size_t encode_into_unchecked(char32_t rune, std::wstring &ws) {
  if (rune <= 0xFFFF) {
    ws += rune_is_surrogate(rune) ? 0xFFFD : static_cast<wchar_t>(rune);
    return 1;
  }
  if (rune > 0x0010FFFF) {
    ws += 0xFFFD;
    return 1;
  }
  ws += static_cast<wchar_t>(0xD7C0 + (rune >> 10));
  ws += static_cast<wchar_t>(0xDC00 + (rune & 0x3FF));
  return 2;
}

[[maybe_unused]] constexpr uint32_t MAX_BARLENGTH = 256;
enum ProgressState : uint32_t { Uninitialized = 0, Running = 33, Completed = 32, Fault = 31 };
class ProgressBar {
public:
  ProgressBar() = default;
  ProgressBar(const ProgressBar &) = delete;
  ProgressBar &operator=(const ProgressBar &) = delete;
  void Maximum(uint64_t mx) { maximum = mx; }
  void Update(uint64_t value) { total = value; }
  void FileName(std::wstring_view fn) {
    if (has_overbyte(fn)) {
      encode_into_unicode(fn, u32name, u32nameLen);
      if (u32nameLen < 19) {
        u32name_shadow = fn;
        u32name_shadow.append(19 - u32nameLen, L' ');
      }
      return;
    }
    filename = fn;
    if (filename.size() >= 20) {
      flen = filename.size();
      filename.append(flen, L' ');
    }
  }

  bool Execute();
  void Finish();
  void MarkFault() { state = ProgressState::Fault; }
  void MarkCompleted() { state = ProgressState::Completed; }
  uint32_t Columns() const { return termsz.columns; }
  uint32_t Rows() const { return termsz.rows; }

private:
  std::atomic_uint64_t maximum{0};
  uint64_t previous{0};
  mutable std::condition_variable cv;
  mutable std::mutex mtx;
  std::shared_ptr<std::thread> worker;
  std::atomic_uint64_t total{0};
  std::atomic_bool active{true};
  std::wstring space;
  std::wstring scs;
  std::wstring filename;
  std::wstring u32name_shadow;
  characters u32name;
  size_t u32nameLen{0};
  std::atomic_uint32_t state{ProgressState::Uninitialized};
  wchar_t speed[64];
  size_t fnpos{0};
  uint32_t tick{0};
  uint32_t pos{0};
  bela::terminal::terminal_size termsz;
  size_t flen{20};
  bool cygwinterminal{false};
  inline std::wstring_view MakeSpace(size_t n) {
    if (n > MAX_BARLENGTH) {
      return std::wstring_view{};
    }
    return std::wstring_view{space.data(), n};
  }
  inline std::wstring_view MakeRate(size_t n) {
    if (n > MAX_BARLENGTH) {
      return std::wstring_view{};
    }
    return std::wstring_view{scs.data(), n};
  }
  inline std::wstring_view ansi_range_name() {
    if (filename.size() <= 19) {
      return filename;
    }
    std::wstring_view sv{filename.data() + fnpos, 19};
    fnpos++;
    if (fnpos == flen) {
      fnpos = 0;
    }
    return sv;
  }
  inline std::wstring_view unicode_range_name() {
    u32name_shadow.clear();
    size_t length = 0;
    auto concat = [&](character &ch) -> bool {
      if (length + ch.width > 19) {
        return false;
      }
      encode_into_unchecked(ch.rune, u32name_shadow);
      length += ch.width;
      if (length == 19) {
        return false;
      }
      return true;
    };
    size_t i = fnpos;
    for (; i < u32name.size(); i++) {
      auto ch = u32name[i];
      if (!concat(ch)) {
        break;
      }
    }
    if (length < 19) {
      u32name_shadow.append(19 - length, L' ');
    }
    fnpos++;
    if (fnpos >= u32name.size()) {
      fnpos = 0;
    }
    return u32name_shadow;
  }

  inline std::wstring_view MakeFileName() {
    if (u32name.empty()) {
      return ansi_range_name();
    }
    if (u32nameLen < 19) {
      return u32name_shadow;
    }
    return unicode_range_name();
  }

  void Loop();
  void Draw();
};
bool CygwinTerminalSize(bela::terminal::terminal_size &termsz);
} // namespace baulk

#endif
