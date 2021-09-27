//
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <atomic>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <baulk/net.hpp>

namespace baulk::net {
// WSAConnectByName
// https://docs.microsoft.com/zh-cn/windows/win32/api/winsock2/nf-winsock2-wsaconnectbynamew
// RIO
// https://docs.microsoft.com/zh-cn/windows/win32/api/mswsock/ns-mswsock-rio_extension_function_table
std::once_flag winsock_once;
class winsock_initializer {
public:
  winsock_initializer() {
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (auto err = WSAStartup(wVersionRequested, &wsaData); err != 0) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"BUGS WSAStartup %s\n", ec.message);
      return;
    }
    initialized = true;
  }
  ~winsock_initializer() {
    if (initialized) {
      WSACleanup();
    }
  }

private:
  std::atomic_bool initialized{false};
};

inline constexpr bool InProgress(int rv) { return rv == WSAEWOULDBLOCK || rv == WSAEINPROGRESS; }

inline bela::error_code make_wsa_error_code(int code, std::wstring_view prefix = L"") {
  bela::error_code ec;
  ec.code = code;
  ec.message = bela::resolve_system_error_message(ec.code, prefix);
  return ec;
}

typedef struct _QueryContext {
  OVERLAPPED QueryOverlapped;
  PADDRINFOEX QueryResults{nullptr};
  HANDLE CompleteEvent{nullptr};
} QUERY_CONTEXT, *PQUERY_CONTEXT;

void WINAPI QueryCompleteCallback(_In_ DWORD Error, _In_ DWORD Bytes, _In_ LPOVERLAPPED Overlapped) {
  PQUERY_CONTEXT QueryContext = nullptr;
  PADDRINFOEX QueryResults = nullptr;

  UNREFERENCED_PARAMETER(Bytes);

  QueryContext = CONTAINING_RECORD(Overlapped, QUERY_CONTEXT, QueryOverlapped);
  //
  //  Notify caller that the query completed
  //
  SetEvent(QueryContext->CompleteEvent);
  return;
}
// query dns timeout use IOCP
// https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-ADDRINFOEX4
// https://github.com/microsoft/Windows-Classic-Samples/blob/master/Samples/DNSAsyncNetworkNameResolution/cpp/ResolveName.cpp
// ADDRINFOEX6 support Windows 11 sdk or later
bool ResolveName(std::wstring_view host, int port, PADDRINFOEX4 *rhints, bela::error_code &ec) {
  ADDRINFOEX4 hints = {0};
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_EXTENDED | AI_FQDN | AI_CANONNAME | AI_RESOLUTION_HANDLE;
  hints.ai_version = ADDRINFOEX_VERSION_6;
  DWORD QueryTimeout = 5 * 1000; // 5 seconds
  QUERY_CONTEXT QueryContext;
  HANDLE CancelHandle = NULL;
  ZeroMemory(&QueryContext, sizeof(QueryContext));
  if (QueryContext.CompleteEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr); QueryContext.CompleteEvent == nullptr) {
    ec = bela::make_system_error_code(L"ResolveName.CreateEvent() ");
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(QueryContext.CompleteEvent); });
  if (auto error = GetAddrInfoExW(host.data(), bela::AlphaNum(port).data(), NS_DNS, NULL,
                                  reinterpret_cast<const ADDRINFOEXW *>(&hints), &QueryContext.QueryResults, NULL,
                                  &QueryContext.QueryOverlapped, QueryCompleteCallback, &CancelHandle);
      error != WSA_IO_PENDING) {
    ec = make_wsa_error_code(WSAGetLastError(), L"GetAddrInfoExW() ");
    QueryCompleteCallback(error, 0, &QueryContext.QueryOverlapped);
    return false;
  }
  if (WaitForSingleObject(QueryContext.CompleteEvent, QueryTimeout) == WAIT_TIMEOUT) {
    GetAddrInfoExCancel(&CancelHandle);
    WaitForSingleObject(QueryContext.CompleteEvent, INFINITE);
    if (QueryContext.QueryResults != nullptr) {
      FreeAddrInfoExW(QueryContext.QueryResults);
    }
    ec = bela::make_error_code(bela::ErrGeneral, L"GetAddrInfoEx() timeout");
    return false;
  }
  if (QueryContext.QueryResults == nullptr) {
    ec = bela::make_error_code(bela::ErrGeneral, L"GetAddrInfoEx() failed");
    return false;
  }
  *rhints = reinterpret_cast<PADDRINFOEX4>(QueryContext.QueryResults);
  return true;
}

void Conn::Close() {
  if (sock != BAULK_INVALID_SOCKET) {
    closesocket(sock);
    sock = BAULK_INVALID_SOCKET;
  }
}

ssize_t Conn::WriteTimeout(const void *data, uint32_t len, int timeout) {
  WSABUF wsabuf{static_cast<ULONG>(len), const_cast<char *>(reinterpret_cast<const char *>(data))};
  DWORD dwbytes = 0;
  if (auto rv = WSASend(sock, &wsabuf, 1, &dwbytes, 0, nullptr, nullptr); rv != SOCKET_ERROR) {
    return static_cast<ssize_t>(dwbytes);
  }
  if (auto rv = WSAGetLastError(); !InProgress(rv)) {
    return -1;
  }
  WSAPOLLFD pfd;
  pfd.fd = sock;
  pfd.events = POLLOUT;
  if (auto rc = WSAPoll(&pfd, 1, timeout); rc <= 0) {
    return -1;
  }
  if (auto rv = WSASend(sock, &wsabuf, 1, &dwbytes, 0, nullptr, nullptr); rv != SOCKET_ERROR) {
    return dwbytes;
  }
  return -1;
}
ssize_t Conn::ReadTimeout(char *buf, size_t len, int timeout) {
  WSABUF wsabuf{static_cast<ULONG>(len), buf};
  DWORD dwbytes = 0;
  DWORD flags = 0;

  if (auto rv = WSARecv(sock, &wsabuf, 1, &dwbytes, &flags, nullptr, nullptr); rv != SOCKET_ERROR) {
    return dwbytes;
  }
  if (auto rv = WSAGetLastError(); InProgress(rv)) {
    return -1;
  }
  WSAPOLLFD pfd;
  pfd.fd = sock;
  pfd.events = POLLIN;
  if (auto rc = WSAPoll(&pfd, 1, timeout); rc <= 0) {
    return -1;
  }
  if (auto rv = WSARecv(sock, &wsabuf, 1, &dwbytes, &flags, nullptr, nullptr); rv != SOCKET_ERROR) {
    return dwbytes;
  }
  return -1;
}

bool DialTimeoutInternal(BAULKSOCK sock, const ADDRINFOEX4 *hi, int timeout, bela::error_code &ec) {
  ULONG flags = 1;
  if (ioctlsocket(sock, FIONBIO, &flags) == SOCKET_ERROR) {
    ec = make_wsa_error_code(WSAGetLastError(), L"ioctlsocket() ");
    return false;
  }
  if (connect(sock, hi->ai_addr, static_cast<int>(hi->ai_addrlen)) != SOCKET_ERROR) {
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
  auto rc = WSAPoll(&pfd, 1, timeout);
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

std::optional<Conn> DialTimeout(std::wstring_view address, int port, int timeout, bela::error_code &ec) {
  static winsock_initializer initializer_;
  PADDRINFOEX4 rhints = nullptr;
  if (!ResolveName(address, port, &rhints, ec)) {
    bela::FPrintF(stderr, L"GetAddrInfoExW %s\n", ec.message);
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
      ec = bela::make_error_code(bela::ErrGeneral, L"connect to ", address, L" timeout");
    }
    FreeAddrInfoExW(reinterpret_cast<ADDRINFOEXW *>(rhints)); /// Release
    return std::nullopt;
  }
  FreeAddrInfoExW(reinterpret_cast<ADDRINFOEXW *>(rhints)); /// Release
  return std::make_optional<baulk::net::Conn>(sock);
}
} // namespace baulk::net
