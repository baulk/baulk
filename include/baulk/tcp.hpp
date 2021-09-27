//
#ifndef BAULK_TCP_HPP
#define BAULK_TCP_HPP
#include <bela/base.hpp>
#include <chrono>

namespace baulk::net {
using BAULKSOCK = UINT_PTR;
constexpr auto BAULK_INVALID_SOCKET = (BAULKSOCK)(~0);
using ssize_t = intptr_t;
class Conn {
public:
  Conn() = default;
  Conn(BAULKSOCK sock_) : sock(sock_) {}
  Conn(Conn &&other) { Move(std::move(other)); }
  Conn &operator=(Conn &&other) {
    Move(std::move(other));
    return *this;
  }
  Conn(const Conn &) = delete;
  Conn &operator=(const Conn &) = delete;
  ~Conn() { Close(); }
  void Close();
  ssize_t WriteTimeout(const void *data, uint32_t len, int timeout);
  ssize_t ReadTimeout(char *buf, size_t len, int timeout);
  BAULKSOCK FD() const { return sock; }

private:
  BAULKSOCK sock{BAULK_INVALID_SOCKET};
  void Move(Conn &&other) {
    Close();
    sock = other.sock;
    other.sock = BAULK_INVALID_SOCKET;
  }
};
// timeout milliseconds
std::optional<Conn> DialTimeout(std::wstring_view address, int port, int timeout,
                                bela::error_code &ec); // second
} // namespace baulk::net

#endif