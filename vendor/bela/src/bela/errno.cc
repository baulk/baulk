// Windows ERROR
#include <bela/base.hpp>
/*
from: C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\ucrt\errno.h
#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define EDEADLK         36
#define ENAMETOOLONG    38
#define ENOLCK          39
#define ENOSYS          40
#define ENOTEMPTY       41

// Error codes used in the Secure CRT functions
#ifndef RC_INVOKED
    #define _SECURECRT_ERRCODE_VALUES_DEFINED
    #define EINVAL          22
    #define ERANGE          34
    #define EILSEQ          42
    #define STRUNCATE       80
#endif

#define EADDRINUSE      100
#define EADDRNOTAVAIL   101
#define EAFNOSUPPORT    102
#define EALREADY        103
#define EBADMSG         104
#define ECANCELED       105
#define ECONNABORTED    106
#define ECONNREFUSED    107
#define ECONNRESET      108
#define EDESTADDRREQ    109
#define EHOSTUNREACH    110
#define EIDRM           111
#define EINPROGRESS     112
#define EISCONN         113
#define ELOOP           114
#define EMSGSIZE        115
#define ENETDOWN        116
#define ENETRESET       117
#define ENETUNREACH     118
#define ENOBUFS         119
#define ENODATA         120
#define ENOLINK         121
#define ENOMSG          122
#define ENOPROTOOPT     123
#define ENOSR           124
#define ENOSTR          125
#define ENOTCONN        126
#define ENOTRECOVERABLE 127
#define ENOTSOCK        128
#define ENOTSUP         129
#define EOPNOTSUPP      130
#define EOTHER          131
#define EOVERFLOW       132
#define EOWNERDEAD      133
#define EPROTO          134
#define EPROTONOSUPPORT 135
#define EPROTOTYPE      136
#define ETIME           137
#define ETIMEDOUT       138
#define ETXTBSY         139
#define EWOULDBLOCK     140

*/
namespace bela {
namespace errno_internal {
// C:/Program Files (x86)/Windows
// Kits/10/Source/10.0.19041.0/ucrt/misc/syserr.cpp
constexpr std::wstring_view errorlist[] = {
    /*  0              */ L"No error",
    /*  1 EPERM        */ L"Operation not permitted",
    /*  2 ENOENT       */ L"No such file or directory",
    /*  3 ESRCH        */ L"No such process",
    /*  4 EINTR        */ L"Interrupted function call",
    /*  5 EIO          */ L"Input/output error",
    /*  6 ENXIO        */ L"No such device or address",
    /*  7 E2BIG        */ L"Arg list too long",
    /*  8 ENOEXEC      */ L"Exec format error",
    /*  9 EBADF        */ L"Bad file descriptor",
    /* 10 ECHILD       */ L"No child processes",
    /* 11 EAGAIN       */ L"Resource temporarily unavailable",
    /* 12 ENOMEM       */ L"Not enough space",
    /* 13 EACCES       */ L"Permission denied",
    /* 14 EFAULT       */ L"Bad address",
    /* 15 ENOTBLK      */ L"Unknown error", /* not POSIX */
    /* 16 EBUSY        */ L"Resource device",
    /* 17 EEXIST       */ L"File exists",
    /* 18 EXDEV        */ L"Improper link",
    /* 19 ENODEV       */ L"No such device",
    /* 20 ENOTDIR      */ L"Not a directory",
    /* 21 EISDIR       */ L"Is a directory",
    /* 22 EINVAL       */ L"Invalid argument",
    /* 23 ENFILE       */ L"Too many open files in system",
    /* 24 EMFILE       */ L"Too many open files",
    /* 25 ENOTTY       */ L"Inappropriate I/O control operation",
    /* 26 ETXTBSY      */ L"Unknown error", /* not POSIX */
    /* 27 EFBIG        */ L"File too large",
    /* 28 ENOSPC       */ L"No space left on device",
    /* 29 ESPIPE       */ L"Invalid seek",
    /* 30 EROFS        */ L"Read-only file system",
    /* 31 EMLINK       */ L"Too many links",
    /* 32 EPIPE        */ L"Broken pipe",
    /* 33 EDOM         */ L"Domain error",
    /* 34 ERANGE       */ L"Result too large",
    /* 35 EUCLEAN      */ L"Unknown error", /* not POSIX */
    /* 36 EDEADLK      */ L"Resource deadlock avoided",
    /* 37 UNKNOWN      */ L"Unknown error",
    /* 38 ENAMETOOLONG */ L"Filename too long",
    /* 39 ENOLCK       */ L"No locks available",
    /* 40 ENOSYS       */ L"Function not implemented",
    /* 41 ENOTEMPTY    */ L"Directory not empty",
    /* 42 EILSEQ       */ L"Illegal byte sequence",
    /* 43              */ L"Unknown error"

};

constexpr std::wstring_view _sys_posix_errlist[] = {
    /* 100 EADDRINUSE      */ L"address in use",
    /* 101 EADDRNOTAVAIL   */ L"address not available",
    /* 102 EAFNOSUPPORT    */ L"address family not supported",
    /* 103 EALREADY        */ L"connection already in progress",
    /* 104 EBADMSG         */ L"bad message",
    /* 105 ECANCELED       */ L"operation canceled",
    /* 106 ECONNABORTED    */ L"connection aborted",
    /* 107 ECONNREFUSED    */ L"connection refused",
    /* 108 ECONNRESET      */ L"connection reset",
    /* 109 EDESTADDRREQ    */ L"destination address required",
    /* 110 EHOSTUNREACH    */ L"host unreachable",
    /* 111 EIDRM           */ L"identifier removed",
    /* 112 EINPROGRESS     */ L"operation in progress",
    /* 113 EISCONN         */ L"already connected",
    /* 114 ELOOP           */ L"too many symbolic link levels",
    /* 115 EMSGSIZE        */ L"message size",
    /* 116 ENETDOWN        */ L"network down",
    /* 117 ENETRESET       */ L"network reset",
    /* 118 ENETUNREACH     */ L"network unreachable",
    /* 119 ENOBUFS         */ L"no buffer space",
    /* 120 ENODATA         */ L"no message available",
    /* 121 ENOLINK         */ L"no link",
    /* 122 ENOMSG          */ L"no message",
    /* 123 ENOPROTOOPT     */ L"no protocol option",
    /* 124 ENOSR           */ L"no stream resources",
    /* 125 ENOSTR          */ L"not a stream",
    /* 126 ENOTCONN        */ L"not connected",
    /* 127 ENOTRECOVERABLE */ L"state not recoverable",
    /* 128 ENOTSOCK        */ L"not a socket",
    /* 129 ENOTSUP         */ L"not supported",
    /* 130 EOPNOTSUPP      */ L"operation not supported",
    /* 131 EOTHER          */ L"Unknown error",
    /* 132 EOVERFLOW       */ L"value too large",
    /* 133 EOWNERDEAD      */ L"owner dead",
    /* 134 EPROTO          */ L"protocol error",
    /* 135 EPROTONOSUPPORT */ L"protocol not supported",
    /* 136 EPROTOTYPE      */ L"wrong protocol type",
    /* 137 ETIME           */ L"stream timeout",
    /* 138 ETIMEDOUT       */ L"timed out",
    /* 139 ETXTBSY         */ L"text file busy",
    /* 140 EWOULDBLOCK     */ L"operation would block",
    /* 141                 */ L"Unknown error"

};

} // namespace errno_internal

std::wstring resolve_system_error_message(DWORD ec, std::wstring_view prefix) {
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, ec,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, nullptr);
  if (rl == 0) {
    return bela::StringCat(prefix, L"GetLastError(): ", ec);
  }
  if (buf[rl - 1] == '\n') {
    rl--;
  }
  if (rl > 0 && buf[rl - 1] == '\r') {
    rl--;
  }
  auto msg = bela::StringCat(prefix, std::wstring_view(buf, rl));
  LocalFree(buf);
  return msg;
}

std::wstring resolve_module_error_message(const wchar_t *moduleName, DWORD ec, std::wstring_view prefix) {
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER, GetModuleHandleW(moduleName),
                           ec, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, nullptr);
  if (rl == 0) {
    return bela::StringCat(prefix, L"GetLastError(): ", ec);
  }
  if (buf[rl - 1] == '\n') {
    rl--;
  }
  if (rl > 0 && buf[rl - 1] == '\r') {
    rl--;
  }
  auto msg = bela::StringCat(prefix, std::wstring_view(buf, rl));
  LocalFree(buf);
  return msg;
}

bela::error_code make_error_code_from_errno(errno_t eno, std::wstring_view prefix) {
  if (eno >= EADDRINUSE && eno <= EWOULDBLOCK) {
    return bela::error_code{bela::StringCat(prefix, errno_internal::_sys_posix_errlist[eno - EADDRINUSE]), eno};
  }
  constexpr auto n = std::size(errno_internal::errorlist);
  auto msg = (static_cast<std::size_t>(eno) >= n) ? errno_internal::errorlist[n - 1] : errno_internal::errorlist[eno];
  return bela::error_code{bela::StringCat(prefix, msg), eno};
}

bela::error_code make_error_code_from_std(const std::error_code &e, std::wstring_view prefix) {
  error_code ec;
  ec.code = e.value();
  ec.message = bela::StringCat(prefix, fromascii(e.message()));
  return ec;
}

} // namespace bela