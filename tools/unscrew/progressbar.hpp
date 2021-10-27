//
#ifndef BAULK_UNSCREW_HPP
#define BAULK_UNSCREW_HPP
#include <bela/base.hpp>
#include <mutex>
#include <thread>

namespace baulk::unscrew {
class ProgressBar {
public:
  ProgressBar() = default;
  ProgressBar(const ProgressBar &) = delete;
  ProgressBar &operator=(const ProgressBar &) = delete;
  ~ProgressBar();
  bool NewProgressBar(std::wstring_view path,bela::error_code &ec);
  bool SetProgress(int64_t completed);

private:
  std::mutex mtx;
  std::shared_ptr<std::thread> ui;
  HWND hProgressBar{nullptr};
  int64_t size{0};
};
} // namespace baulk::unscrew

#endif