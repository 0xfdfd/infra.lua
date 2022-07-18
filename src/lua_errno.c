#include "lua_errno.h"
#include <errno.h>

const char* infra_strerror_c(int errcode)
{
#define EXPAND_INFRA_ERROR_AS_STRING(err, val, str)  \
    case err: return str;

    switch(errcode)
    {
    INFRA_ERRNO(EXPAND_INFRA_ERROR_AS_STRING)
    default:            return NULL;
    }

#undef EXPAND_INFRA_ERROR_AS_STRING
}

int infra_strerror(lua_State* L)
{
    int errcode = (int)luaL_checkinteger(L, 1);
    const char* err_str = infra_strerror_c(errcode);

    if (err_str == NULL)
    {
        lua_pushnil(L);
    }
    else
    {
        lua_pushstring(L, err_str);
    }

    return 1;
}

int infra_translate_sys_error(int syserr)
{
    switch (syserr)
    {
        /* Success */
        case 0:                 return INFRA_SUCCESS;
        /* Posix */
        case EPERM:             return INFRA_EPERM;
        case ENOENT:            return INFRA_ENOENT;
        case EIO:               return INFRA_EIO;
        case E2BIG:             return INFRA_E2BIG;
        case EBADF:             return INFRA_EBADF;
        case EAGAIN:            return INFRA_EAGAIN;
        case ENOMEM:            return INFRA_ENOMEM;
        case EACCES:            return INFRA_EACCES;
        case EFAULT:            return INFRA_EFAULT;
        case EBUSY:             return INFRA_EBUSY;
        case EEXIST:            return INFRA_EEXIST;
        case EXDEV:             return INFRA_EXDEV;
        case ENOTDIR:           return INFRA_ENOTDIR;
        case EISDIR:            return INFRA_EISDIR;
        case EINVAL:            return INFRA_EINVAL;
        case EMFILE:            return INFRA_EMFILE;
        case ENOSPC:            return INFRA_ENOSPC;
        case EROFS:             return INFRA_EROFS;
        case EPIPE:             return INFRA_EPIPE;
        case ENAMETOOLONG:      return INFRA_ENAMETOOLONG;
        case ENOTEMPTY:         return INFRA_ENOTEMPTY;
        case EADDRINUSE:        return INFRA_EADDRINUSE;
        case EADDRNOTAVAIL:     return INFRA_EADDRNOTAVAIL;
        case EAFNOSUPPORT:      return INFRA_EAFNOSUPPORT;
        case EALREADY:          return INFRA_EALREADY;
        case ECANCELED:         return INFRA_ECANCELED;
        case ECONNABORTED:      return INFRA_ECONNABORTED;
        case ECONNREFUSED:      return INFRA_ECONNREFUSED;
        case ECONNRESET:        return INFRA_ECONNRESET;
        case EHOSTUNREACH:      return INFRA_EHOSTUNREACH;
        case EINPROGRESS:       return INFRA_EINPROGRESS;
        case EISCONN:           return INFRA_EISCONN;
        case ELOOP:             return INFRA_ELOOP;
        case EMSGSIZE:          return INFRA_EMSGSIZE;
        case ENETUNREACH:       return INFRA_ENETUNREACH;
        case ENOBUFS:           return INFRA_ENOBUFS;
        case ENOTCONN:          return INFRA_ENOTCONN;
        case ENOTSOCK:          return INFRA_ENOTSOCK;
        case ENOTSUP:           return INFRA_ENOTSUP;
        case EPROTO:            return INFRA_EPROTO;
        case EPROTONOSUPPORT:   return INFRA_EPROTONOSUPPORT;
        case ETIMEDOUT:         return INFRA_ETIMEDOUT;
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:       return INFRA_EAGAIN;
#endif
        /* Unknown */
        default:                break;
    }

    return syserr;
}
