#include <algorithm>
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/escapeargv.hpp>
#include <bela/process.hpp>
#include <bela/charconv.hpp>
#include <baulk/indicators.hpp>
// #include <format> Waiting C++20 <format> final

namespace baulk {
[[maybe_unused]] constexpr uint64_t KB = 1024ULL;
[[maybe_unused]] constexpr uint64_t MB = KB * 1024;
[[maybe_unused]] constexpr uint64_t GB = MB * 1024;
[[maybe_unused]] constexpr uint64_t TB = GB * 1024;

template <size_t N> inline std::wstring_view to_chars(wchar_t (&buf)[N], double v, wchar_t la) {
  if (auto result = bela::to_chars(buf, buf + N, v, std::chars_format::fixed, 2); result.ec == std::errc{}) {
    if (result.ptr < buf + N) {
      *result.ptr++ = la;
    }
    return std::wstring_view{buf, static_cast<size_t>(result.ptr - buf)};
  }
  return L"";
}

template <size_t N> inline std::wstring_view encode_rate(wchar_t (&buf)[N], uint64_t x) {
  if (x >= TB) {
    return to_chars(buf, (double)x / TB, 'T');
  }
  if (x >= GB) {
    return to_chars(buf, (double)x / GB, 'G');
  }
  if (x >= MB) {
    return to_chars(buf, (double)x / MB, 'M');
  }
  if (x > 10 * KB) {
    return to_chars(buf, (double)x / KB, 'K');
  }
  if (auto result = bela::to_chars(buf, x); result.ec == std::errc{}) {
    if (result.ptr < buf + N) {
      *result.ptr++ = 'B';
    }
    return std::wstring_view{buf, static_cast<size_t>(result.ptr - buf)};
  }
  return L"";
}

// CygwinTerminalSize resolve cygwin terminal size use stty,
// When running under Cygwin terminal, stty should be available
bool CygwinTerminalSize(bela::terminal::terminal_size &termsz) {
  bela::process::Process ps;
  constexpr DWORD flags = bela::process::CAPTURE_USEIN | bela::process::CAPTURE_USEERR;
  if (auto exitcode = ps.CaptureWithMode(flags, L"stty", L"size"); exitcode != 0) {
    bela::FPrintF(stderr, L"stty %d: %s\n", exitcode, ps.ErrorCode().message);
    return false;
  }
  auto out = bela::encode_into<char, wchar_t>(ps.Out());
  std::vector<std::wstring_view> ss =
      bela::StrSplit(bela::StripTrailingAsciiWhitespace(out), bela::ByChar(' '), bela::SkipEmpty());
  if (ss.size() != 2) {
    return false;
  }
  if (!bela::SimpleAtoi(ss[0], &termsz.rows)) {
    return false;
  }
  if (!bela::SimpleAtoi(ss[1], &termsz.columns)) {
    return false;
  }
  termsz.columns = std::max<uint32_t>(termsz.columns, 80);
  return true;
}

void ProgressBar::Loop() {
  while (active) {
    {
      std::unique_lock lock(mtx);
      cv.wait_for(lock, std::chrono::milliseconds(100));
    }
    // --- draw progress bar
    Draw();
  }
  bela::FPrintF(stderr, L"\n");
}

void ProgressBar::Draw() {
  if (!cygwinterminal) {
    bela::terminal::TerminalSize(stderr, termsz);
    termsz.columns = std::max<uint32_t>(termsz.columns, 80);
  }

  // file.tar.gz 17%[###############>      ] 1024.00K 1024.00K/s
  auto barwidth = termsz.columns - 50;
  wchar_t strtotal[64];
  auto total_ = static_cast<uint64_t>(total);
  //' 1024.00K 1024.00K/s' 20
  auto rs = encode_rate(strtotal, total_);
  if (tick % 10 == 0) {
    auto delta = (total_ - previous); // cycle 50/1000 s
    previous = total_;
    if (auto sv = encode_rate(speed, delta); sv.size() < std::size(speed)) {
      speed[sv.size()] = 0;
    }
  }
  tick++;
  auto maximum_ = static_cast<std::uint64_t>(maximum);
  if (maximum_ == 0) {
    barwidth += 5;
    // file.tar.gz  [ <=> ] 1024.00K 1024.00K/s
    constexpr std::wstring_view bounce = L"<=>";
    pos++;
    if (pos == barwidth - 3) {
      pos = 0;
    }
    //[##############################]
    //[      <=>                     ]
    // '<=>'
    auto s0 = MakeSpace(pos);
    auto s1 = MakeSpace(barwidth - pos - 3);
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[%dm%s [%s%s%s] %s %s/s\x1b[0m", (uint32_t)state, MakeFileName(), s0, bounce,
                  s1, rs, speed);
    return;
  }
  auto scale = total_ * 100 / maximum_;
  auto progress = scale * barwidth / 100;
  auto ps = MakeRate(static_cast<size_t>(progress));
  auto sps = MakeSpace(static_cast<size_t>(barwidth - progress));
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[%dm%s %d%% [%s%s] %s %s/s\x1b[0m", (uint32_t)state, MakeFileName(), scale, ps,
                sps, rs, speed);
}

bool ProgressBar::Execute() {
  // IsSameTerminal check is terminal
  if (!bela::terminal::IsSameTerminal(stderr)) {
    return false;
  }
  if (cygwinterminal = bela::terminal::IsCygwinTerminal(stderr); cygwinterminal) {
    CygwinTerminalSize(termsz);
  }
  state = ProgressState::Running;
  worker = std::make_shared<std::thread>([this] {
    // Progress Loop
    space.resize(MAX_BARLENGTH + 4, L' ');
    scs.resize(MAX_BARLENGTH + 4, L'#');
    memset(speed, 0, sizeof(speed));
    this->Loop();
  });
  return true;
}
void ProgressBar::Finish() {
  if (state == ProgressState::Uninitialized) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mtx);
    active = false;
  }
  cv.notify_all();
  worker->join();
}

}; // namespace baulk
