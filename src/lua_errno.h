#ifndef __INFRA_API_ERRNO_H__
#define __INFRA_API_ERRNO_H__

#include "lua_api.h"

#define INFRA_LUA_API_STRERROR(XX)                                                      \
XX(                                                                                     \
"strerror", infra_strerror, NULL,                                                       \
"return string describing error number",                                                \
"SYNOPSIS"                                                                          "\n"\
"    string strerror(errcode);"                                                     "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `strerror()` function returns a string that describes the error code"      "\n"\
"    passed in the argument `errcode`."                                             "\n"\
                                                                                    "\n"\
"    In infra library, all error code is a negative integer, which allow you to"    "\n"\
"    easily known whether an operation is a success or not."                        "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    On success, a string will be returned. On error, nil will be returned."        "\n"\
)

#define INFRA_ERRNO(XX) \
    XX(INFRA_SUCCESS,           0,      "Operation success")                        \
    XX(INFRA_EPERM,             -1,     "Operation not permitted")                  \
    XX(INFRA_ENOENT,            -2,     "No such file or directory")                \
    XX(INFRA_EIO,               -5,     "Input/output error")                       \
    XX(INFRA_E2BIG,             -7,     "Argument list too long")                   \
    XX(INFRA_EBADF,             -9,     "Bad file descriptor")                      \
    XX(INFRA_EAGAIN,            -11,    "Resource temporarily unavailable")         \
    XX(INFRA_ENOMEM,            -12,    "Not enough space/cannot allocate memory")  \
    XX(INFRA_EACCES,            -13,    "Permission denied")                        \
    XX(INFRA_EFAULT,            -14,    "Bad address")                              \
    XX(INFRA_EBUSY,             -16,    "Device or resource busy")                  \
    XX(INFRA_EEXIST,            -17,    "File exists")                              \
    XX(INFRA_EXDEV,             -18,    "Improper link")                            \
    XX(INFRA_ENOTDIR,           -20,    "Not a directory")                          \
    XX(INFRA_EISDIR,            -21,    "Is a directory")                           \
    XX(INFRA_EINVAL,            -22,    "Invalid argument")                         \
    XX(INFRA_EMFILE,            -24,    "Too many open files")                      \
    XX(INFRA_ENOSPC,            -28,    "No space left on device")                  \
    XX(INFRA_EROFS,             -30,    "Read-only filesystem")                     \
    XX(INFRA_EPIPE,             -32,    "Broken pipe")                              \
    XX(INFRA_ENAMETOOLONG,      -36,    "Filename too long")                        \
    XX(INFRA_ENOTEMPTY,         -39,    "Directory not empty")                      \
    XX(INFRA_ELOOP,             -40,    "Too many levels of symbolic links")        \
    XX(INFRA_EPROTO,            -71,    "Protocol error")                           \
    XX(INFRA_ENOTSOCK,          -88,    "Not a socket")                             \
    XX(INFRA_EMSGSIZE,          -90,    "Message too long")                         \
    XX(INFRA_EPROTONOSUPPORT,   -93,    "Protocol not supported")                   \
    XX(INFRA_EAFNOSUPPORT,      -97,    "Address family not supported")             \
    XX(INFRA_EADDRINUSE,        -98,    "Address already in use")                   \
    XX(INFRA_EADDRNOTAVAIL,     -99,    "Address not available")                    \
    XX(INFRA_ENETUNREACH,       -101,   "Network unreachable")                      \
    XX(INFRA_ECONNABORTED,      -103,   "Connection aborted")                       \
    XX(INFRA_ECONNRESET,        -104,   "Connection reset")                         \
    XX(INFRA_ENOBUFS,           -105,   "No buffer space available")                \
    XX(INFRA_EISCONN,           -106,   "Socket is connected")                      \
    XX(INFRA_ENOTCONN,          -107,   "The socket is not connected")              \
    XX(INFRA_ETIMEDOUT,         -110,   "Connection timed out")                     \
    XX(INFRA_ECONNREFUSED,      -111,   "Connection refused")                       \
    XX(INFRA_EHOSTUNREACH,      -113,   "Host is unreachable")                      \
    XX(INFRA_EALREADY,          -114,   "Connection already in progress")           \
    XX(INFRA_EINPROGRESS,       -115,   "Operation in progress")                    \
    XX(INFRA_ECANCELED,         -125,   "Operation canceled")                       \
    XX(INFRA_ENOTSUP,           -134,   "Operation not supported")

#ifdef __cplusplus
extern "C" {
#endif

typedef enum infra_errno
{
#define EXPAND_ERRNO_AS_ENUM(err, val, str) err = val,
    INFRA_ERRNO(EXPAND_ERRNO_AS_ENUM)
#undef EXPAND_ERRNO_AS_ENUM
} infra_errno_t;

/**
 * @see INFRA_LUA_API_STRERROR
 */
API_LOCAL int infra_strerror(lua_State* L);

API_LOCAL const char* infra_strerror_c(int errcode);

API_LOCAL int infra_translate_sys_error(int syserr);

#ifdef __cplusplus
}
#endif

#endif
