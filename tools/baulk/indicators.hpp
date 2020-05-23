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
[[maybe_unused]] constexpr uint64_t KB = 1024ULL;
[[maybe_unused]] constexpr uint64_t MB = KB * 1024;
[[maybe_unused]] constexpr uint64_t GB = MB * 1024;
[[maybe_unused]] constexpr uint64_t TB = GB * 1024;
[[maybe_unused]] constexpr uint32_t MAX_BARLENGTH = 256;
template <size_t N> void EncodeRate(wchar_t (&buf)[N], uint64_t x) {
  if (x >= TB) {
    _snwprintf_s(buf, N, L"%.2fT", (double)x / TB);
    return;
  }
  if (x >= GB) {
    _snwprintf_s(buf, N, L"%.2fG", (double)x / GB);
    return;
  }
  if (x >= MB) {
    _snwprintf_s(buf, N, L"%.2fM", (double)x / MB);
    return;
  }
  if (x > 10 * KB) {
    _snwprintf_s(buf, N, L"%.2fK", (double)x / KB);
    return;
  }
  _snwprintf_s(buf, N, L"%lldB", x);
}

enum ProgressState : uint32_t {
  Uninitialized = 0,
  Running = 33,
  Completed = 32,
  Fault = 31
};

class ProgressBar {
public:
  ProgressBar() = default;
  ProgressBar(const ProgressBar &) = delete;
  ProgressBar &operator=(const ProgressBar &) = delete;
  void Maximum(uint64_t mx) { maximum = mx; }
  void FileName(std::wstring_view fn) {
    filename = fn;
    if (filename.size() >= 20) {
      flen = filename.size();
      filename.append(flen, L' ');
    }
  }
  void Update(uint64_t value) { total = value; }
  bool Execute();
  void Finish();
  void MarkFault() { state = ProgressState::Fault; }
  void MarkCompleted() { state = ProgressState::Completed; }
  uint32_t Columns() const { return termsz.columns; }
  uint32_t Rows() const { return termsz.rows; }

private:
  uint64_t maximum{0};
  uint64_t previous{0};
  mutable std::condition_variable cv;
  mutable std::mutex mtx;
  std::shared_ptr<std::thread> worker;
  std::atomic_uint64_t total{0};
  std::atomic_bool active{true};
  std::wstring space;
  std::wstring scs;
  std::wstring filename;
  std::atomic_uint32_t state{ProgressState::Uninitialized};
  wchar_t speed[64];
  uint32_t tick{0};
  uint32_t fnpos{0};
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
  inline std::wstring_view MakeFileName() {
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

  void Loop();
  void Draw();
  bool MakeColumns();
};
} // namespace baulk

#endif
