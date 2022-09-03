#ifndef BAULK_ECHO_HPP
#define BAULK_ECHO_HPP
#include <cstdio>
#include <cstdint>
#include <numbers>
#include <bit>
#include <bela/base.hpp>

namespace baulk::echo {
constexpr std::byte packet_end[4] = {std::byte('0'), std::byte('0'), std::byte('0'), std::byte('0')};
constexpr std::int64_t packet_max_len = (65536ll - std::size(packet_end));

// Writer echo writer
class Writer {
public:
  Writer(HANDLE fd_) : fd(fd_) {}

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
};

//
class Reader {
public:
  Reader(HANDLE fd_) : fd(fd_) {}

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
};
} // namespace baulk::echo

#endif