//
#include <bela/base.hpp>
/*
// https://github.com/bminor/musl/blob/master/arch/generic/bits/errno.h
// musl
#define EPERM            1
#define ENOENT           2
#define ESRCH            3
#define EINTR            4
#define EIO              5
#define ENXIO            6
#define E2BIG            7
#define ENOEXEC          8
#define EBADF            9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define ENOTBLK         15
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define ETXTBSY         26
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define ERANGE          34
#define EDEADLK         35
#define ENAMETOOLONG    36
#define ENOLCK          37
#define ENOSYS          38
#define ENOTEMPTY       39
#define ELOOP           40
#define EWOULDBLOCK     EAGAIN
#define ENOMSG          42
#define EIDRM           43
#define ECHRNG          44
#define EL2NSYNC        45
#define EL3HLT          46
#define EL3RST          47
#define ELNRNG          48
#define EUNATCH         49
#define ENOCSI          50
#define EL2HLT          51
#define EBADE           52
#define EBADR           53
#define EXFULL          54
#define ENOANO          55
#define EBADRQC         56
#define EBADSLT         57
#define EDEADLOCK       EDEADLK
#define EBFONT          59
#define ENOSTR          60
#define ENODATA         61
#define ETIME           62
#define ENOSR           63
#define ENONET          64
#define ENOPKG          65
#define EREMOTE         66
#define ENOLINK         67
#define EADV            68
#define ESRMNT          69
#define ECOMM           70
#define EPROTO          71
#define EMULTIHOP       72
#define EDOTDOT         73
#define EBADMSG         74
#define EOVERFLOW       75
#define ENOTUNIQ        76
#define EBADFD          77
#define EREMCHG         78
#define ELIBACC         79
#define ELIBBAD         80
#define ELIBSCN         81
#define ELIBMAX         82
#define ELIBEXEC        83
#define EILSEQ          84
#define ERESTART        85
#define ESTRPIPE        86
#define EUSERS          87
#define ENOTSOCK        88
#define EDESTADDRREQ    89
#define EMSGSIZE        90
#define EPROTOTYPE      91
#define ENOPROTOOPT     92
#define EPROTONOSUPPORT 93
#define ESOCKTNOSUPPORT 94
#define EOPNOTSUPP      95
#define ENOTSUP         EOPNOTSUPP
#define EPFNOSUPPORT    96
#define EAFNOSUPPORT    97
#define EADDRINUSE      98
#define EADDRNOTAVAIL   99
#define ENETDOWN        100
#define ENETUNREACH     101
#define ENETRESET       102
#define ECONNABORTED    103
#define ECONNRESET      104
#define ENOBUFS         105
#define EISCONN         106
#define ENOTCONN        107
#define ESHUTDOWN       108
#define ETOOMANYREFS    109
#define ETIMEDOUT       110
#define ECONNREFUSED    111
#define EHOSTDOWN       112
#define EHOSTUNREACH    113
#define EALREADY        114
#define EINPROGRESS     115
#define ESTALE          116
#define EUCLEAN         117
#define ENOTNAM         118
#define ENAVAIL         119
#define EISNAM          120
#define EREMOTEIO       121
#define EDQUOT          122
#define ENOMEDIUM       123
#define EMEDIUMTYPE     124
#define ECANCELED       125
#define ENOKEY          126
#define EKEYEXPIRED     127
#define EKEYREVOKED     128
#define EKEYREJECTED    129
#define EOWNERDEAD      130
#define ENOTRECOVERABLE 131
#define ERFKILL         132
#define EHWPOISON       133
*/
namespace bela {
namespace errno_internal {
// C:/Program Files (x86)/Windows
// Kits/10/Source/10.0.18362.0/ucrt/misc/syserr.cpp
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
} // namespace errno_internal

std::wstring resolve_system_error_code(DWORD ec) {
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, ec,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&buf, 0, nullptr);
  if (rl == 0) {
    return L"FormatMessageW error";
  }
  if (buf[rl - 1] == '\n') {
    rl--;
  }
  if (rl > 0 && buf[rl - 1] == '\r') {
    rl--;
  }
  std::wstring msg(buf, rl);
  LocalFree(buf);
  return msg;
}

bela::error_code make_stdc_error_code(errno_t eno) {
  constexpr auto n = std::size(errno_internal::errorlist);
  auto msg = eno >= n ? errno_internal::errorlist[n - 1]
                      : errno_internal::errorlist[eno];
  return bela::error_code{std::wstring(msg), eno};
}

} // namespace bela