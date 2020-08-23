//
#ifndef BAULK_NET_HPP
#define BAULK_NET_HPP
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/ascii.hpp>
#include <bela/phmap.hpp>
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
  void Move(Conn &&other);
};
// timeout milliseconds
std::optional<baulk::net::Conn> DialTimeout(std::wstring_view address, int port, int timeout,
                                            bela::error_code &ec); // second
struct StringCaseInsensitiveHash {
  using is_transparent = void;
  std::size_t operator()(std::wstring_view wsv) const noexcept {
    /// See Wikipedia
    /// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
#if defined(__x86_64) || defined(_WIN64)
    static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 14695981039346656037ULL;
    constexpr size_t kFNVPrime = 1099511628211ULL;
#else
    static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 2166136261U;
    constexpr size_t kFNVPrime = 16777619U;
#endif
    size_t val = kFNVOffsetBasis;
    std::string_view sv = {reinterpret_cast<const char *>(wsv.data()), wsv.size() * 2};
    for (auto c : sv) {
      val ^= static_cast<size_t>(bela::ascii_tolower(c));
      val *= kFNVPrime;
    }
    return val;
  }
};

struct StringCaseInsensitiveEq {
  using is_transparent = void;
  bool operator()(std::wstring_view wlhs, std::wstring_view wrhs) const { return bela::EqualsIgnoreCase(wlhs, wrhs); }
};
using headers_t = bela::flat_hash_map<std::wstring, std::wstring, StringCaseInsensitiveHash, StringCaseInsensitiveEq>;
// HTTP Response
enum class Protocol { HTTP11, HTTP20, HTTP30 };
struct Response {
  headers_t hkv;
  std::string body;
  long statuscode{0};
  Protocol protocol{Protocol::HTTP11};
  [[nodiscard]] bool IsSuccessStatusCode() const { return statuscode >= 200 && statuscode <= 299; }
};

class HttpClient {
public:
  HttpClient() = default;
  HttpClient(const HttpClient &) = delete;
  HttpClient &operator=(const HttpClient &) = delete;
  HttpClient &Set(std::wstring_view key, std::wstring_view value) {
    hkv[key] = value;
    return *this;
  }
  HttpClient &AddCookie(std::wstring_view cookie) {
    cookies.emplace_back(cookie);
    return *this;
  }
  std::optional<Response> WinRest(std::wstring_view method, std::wstring_view url, std::wstring_view contenttype,
                                  std::wstring_view body, bela::error_code &ec);
  std::optional<Response> Get(std::wstring_view url, bela::error_code &ec) {
    return WinRest(L"GET", url, L"", L"", ec);
  }
  static HttpClient &DefaultClient() {
    static HttpClient client;
    return client;
  }

private:
  headers_t hkv;
  std::vector<std::wstring> cookies;
};

// RestGet
inline std::optional<Response> RestGet(std::wstring_view url, bela::error_code &ec) {
  return HttpClient::DefaultClient().WinRest(L"GET", url, L"", L"", ec);
}
// Parse http
// download some file to spec workdir
std::optional<std::wstring> WinGet(std::wstring_view url, std::wstring_view workdir, bool forceoverwrite,
                                   bela::error_code &ec);
std::uint64_t UrlResponseTime(std::wstring_view url);
std::wstring_view BestUrl(const std::vector<std::wstring> &urls);
std::wstring_view UrlFileName(std::wstring_view url);
} // namespace baulk::net

#endif
