#ifndef BAULK_ECHO_HPP
#define BAULK_ECHO_HPP
#include <cstdio>
#include <cstdint>
#include <numbers>
#include <bit>
#include <bela/base.hpp>
#include <bela/io.hpp>

namespace baulk::echo {
constexpr uint8_t packet_end[4] = {'0', '0', '0', '0'};
constexpr std::int64_t packet_max_len = (65536ll - std::size(packet_end));

constexpr std::string_view commands[] = {"install", "uninstall", "search", "list",    "version", "info",
                                         "update",  "upgrade",   "freeze", "extract", "bucket"};

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