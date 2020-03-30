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

inline constexpr bool InProgress(int rv) {
  return rv == WSAEWOULDBLOCK || rv == WSAEINPROGRESS;
}

inline error_code make_wsa_error_code(int code,
                                      std::wstring_view prefix = L"") {
  error_code ec;
  ec.code = code;
  ec.message = bela::resolve_system_error_code(ec.code, prefix);
  return ec;
}

bool DialTimeoutInternal(BAULKSOCK sock, const ADDRINFOW *hi, int timeout,
                         bela::error_code &ec) {
  ULONG flags = 1;
  if (ioctlsocket(sock, FIONBIO, &flags) == SOCKET_ERROR) {
    ec = make_wsa_error_code(WSAGetLastError(), L"ioctlsocket() ");
    return false;
  }
  if (connect(sock, hi->ai_addr, static_cast<int>(hi->ai_addrlen)) !=
      SOCKET_ERROR) {
    // success
    return true;
  }
  if (auto rv = WSAGetLastError(); !InProgress(rv)) {
    ec = make_wsa_error_code(rv, L"connect() ");
    return false;
  }
  WSAPOLLFD pfd;
  pfd.fd = sock;
  pfd.events = POLLOUT;
  auto rc = WSAPoll(&pfd, 1, sec * 1000);
  if (rc == 0) {
    // timeout error DialTimeout make it
    return false;
  }
  if (rc < 0) {
    ec = make_wsa_error_code(WSAGetLastError(), L"connect() ");
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
    ec = make_wsa_error_code(L"GetAddrInfoW() ");
    return std::nullopt;
  }
  auto hi = rhints;
  SOCKET sock{BAULK_INVALID_SOCKET};
  do {
    sock = socket(rhints->ai_family, SOCK_STREAM, 0);
    if (sock == BAULK_INVALID_SOCKET) {
      ec = make_wsa_error_code(WSAGetLastError(), L"socket() ");
      continue;
    }
    if (DialTimeoutInternal(sock, hi, timeout, ec)) {
      break;
    }
    closesocket(sock);
    sock = BAULK_INVALID_SOCKET;
  } while ((hi = hi->ai_next) != nullptr);

  if (sock == BAULK_INVALID_SOCKET) {
    if (!ec) {
      ec = bela::make_error_code(1, L"connect to ", address, L" timeout");
    }
    FreeAddrInfoW(rhints); /// Release
    return std::nullopt;
  }
  FreeAddrInfoW(rhints); /// Release
  return std::make_optional<baulk::net::Conn>(sock);
}
} // namespace baulk::net
