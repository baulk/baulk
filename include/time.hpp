//
#ifndef BAULK_TIME_HPP
#define BAULK_TIME_HPP
#include <string>
#include <ctime>
#include <cstdio>

namespace baulk::time {

inline std::string TimeNow() {
  auto t = std::time(nullptr);
  std::tm tm_;
  localtime_s(&tm_, &t);
  std::string buffer;
  buffer.resize(64);
  auto n = std::strftime(buffer.data(), 64, "%Y-%m-%dT%H:%M:%S%z", &tm_);
  buffer.resize(n);
  return buffer;
}

} // namespace baulk::time

#endif