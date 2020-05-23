#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/escapeargv.hpp>
#include <bela/process.hpp>
#include "indicators.hpp"

namespace baulk {

bool ProgressBar::MakeColumns() {
  bela::process::Process ps;
  constexpr DWORD flags =
      bela::process::CAPTURE_USEIN | bela::process::CAPTURE_USEERR;
  if (auto exitcode = ps.CaptureWithMode(flags, L"stty", L"size");
      exitcode != 0) {
    bela::FPrintF(stderr, L"stty %d: %s\n", exitcode, ps.ErrorCode().message);
    return false;
  }
  auto out = bela::ToWide(ps.Out());
  std::vector<std::wstring_view> ss =
      bela::StrSplit(bela::StripTrailingAsciiWhitespace(out), bela::ByChar(' '),
                     bela::SkipEmpty());
  if (ss.size() != 2) {
    return false;
  }
  if (!bela::SimpleAtoi(ss[0], &termsz.rows)) {
    return false;
  }
  if (!bela::SimpleAtoi(ss[1], &termsz.columns)) {
    return false;
  }
  if (termsz.columns < 80) {
    termsz.columns = 80;
  }
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
    if (termsz.columns < 80) {
      termsz.columns = 80;
    }
  }

  // file.tar.gz 17%[###############>      ] 1024.00K 1024.00K/s
  auto barwidth = termsz.columns - 50;
  wchar_t strtotal[64];
  auto total_ = static_cast<uint64_t>(total);
  //' 1024.00K 1024.00K/s' 20

  EncodeRate(strtotal, total_);
  if (tick % 10 == 0) {
    auto delta = (total_ - previous); // cycle 50/1000 s
    previous = total_;
    EncodeRate(speed, delta);
  }
  tick++;
  if (maximum == 0) {
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
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[01;%dm%s [%s%s%s] %s %s/s\x1b[0m",
                  (uint32_t)state, MakeFileName(), s0, bounce, s1, strtotal,
                  speed);
    return;
  }
  auto scale = total_ * 100 / maximum;
  auto progress = scale * barwidth / 100;
  auto ps = MakeRate(static_cast<size_t>(progress));
  auto sps = MakeSpace(static_cast<size_t>(barwidth - progress));
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[01;%dm%s %d%% [%s%s] %s %s/s\x1b[0m",
                (uint32_t)state, MakeFileName(), scale, ps, sps, strtotal,
                speed);
}

bool ProgressBar::Execute() {
  // IsSameTerminal check is terminal
  if (!bela::terminal::IsSameTerminal(stderr)) {
    return false;
  }
  state = ProgressState::Running;
  worker = std::make_shared<std::thread>([this] {
    // Progress Loop
    space.resize(MAX_BARLENGTH + 4, L' ');
    scs.resize(MAX_BARLENGTH + 4, L'#');
    memset(speed, 0, sizeof(speed));
    if (cygwinterminal = bela::terminal::IsCygwinTerminal(stderr);
        cygwinterminal) {
      MakeColumns();
    }
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
