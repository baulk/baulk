//
#include <bela/base.hpp>
#include <bela/stdwriter.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "net.hpp"

namespace baulk::net {
class Winsock {
public:
  Winsock() {
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (auto err = WSAStartup(wVersionRequested, &wsaData); err != 0) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"BUGS WSAStartup %s\n", ec.message);
      return;
    }
    initialized = true;
  }
  ~Winsock() {
    if (initialized) {
      WSACleanup();
    }
  }

private:
  bool initialized{false};
};

Conn::~Conn() {
  if (sock != BAULK_INVALID_SOCKET) {
    closesocket(sock);
  }
}
void Conn::Move(Conn &&other) {
  if (sock != BAULK_INVALID_SOCKET) {
    closesocket(sock);
  }
  sock = other.sock;
  other.sock = BAULK_INVALID_SOCKET;
}

constexpr int POLL_SUCCESS = 0;
constexpr int POLL_TIMEOUT = 1;

int BaulkPoll(SOCKET sock, int nes, bool for_read) {
  WSAPOLLFD pfd;
  pfd.fd = sock;
  pfd.events = for_read ? POLLIN : POLLOUT;
  int rc = 0;
  do {
    rc = WSAPoll(&pfd, 1, nes * 1000);
  } while (rc == -1 && errno == EINTR);
  if (rc == 0) {
    return POLL_TIMEOUT;
  }
  if (rc > 0) {
    return POLL_SUCCESS;
  }
  return -errno;
}

bool ConnectEx(SOCKET sock, const struct sockaddr *name, int namelen, int nes) {
  ULONG flags = 1;
  if (ioctlsocket(sock, FIONBIO, &flags) == SOCKET_ERROR) {
    return false;
  }
  if (connect(sock, name, namelen) != SOCKET_ERROR) {
    return true;
  }
  auto rv = WSAGetLastError();
  if (rv != WSAEWOULDBLOCK && rv != WSAEINPROGRESS) {
    return false;
  }
  rv = BaulkPoll(sock, nes, false);
  if (rv != POLL_SUCCESS) {
    return false;
  }
  rv = WSAGetLastError();
  if (rv != 0 && rv != WSAEISCONN) {
    return false;
  }
  return true;
}

std::optional<baulk::net::Conn> DialTimeout(std::wstring_view address, int port,
                                            int timeout, bela::error_code &ec) {
  static Winsock winsock_;
  ADDRINFOW *rhints = NULL;
  ADDRINFOW hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  if (GetAddrInfoW(address.data(), bela::AlphaNum(port).data(), &hints,
                   &rhints) != 0) {
    ec = bela::make_system_error_code(L"GetAddrInfoW() ");
    return std::nullopt;
  }
  auto hi = rhints;
  SOCKET sock{BAULK_INVALID_SOCKET};
  do {
    sock = socket(rhints->ai_family, SOCK_STREAM, 0);
    if (sock == BAULK_INVALID_SOCKET) {
      continue;
    }
    if (ConnectEx(sock, hi->ai_addr, (int)hi->ai_addrlen, timeout)) {
      break;
    }
    closesocket(sock);
    sock = BAULK_INVALID_SOCKET;
  } while ((hi = hi->ai_next) != nullptr);

  if (sock == BAULK_INVALID_SOCKET) {
    ec = bela::make_error_code(1, L"connect to ", address, L" timeout");
    FreeAddrInfoW(rhints); /// Release
    return std::nullopt;
  }
  FreeAddrInfoW(rhints); /// Release
  return std::make_optional<baulk::net::Conn>(sock);
}
} // namespace baulk::net