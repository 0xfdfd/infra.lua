#include <stdlib.h>
#include "compat_sys.h"

/**
 * @brief Error code map for GetLastError()
 * @see https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
 * @see https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
 */
#define INFRA_SYS_ERRNO_WIN32_MAP(xx)           \
    xx(WAIT_TIMEOUT,        INFRA_ETIMEDOUT)    \
    xx(ERROR_BROKEN_PIPE,   INFRA_EOF)

/**
 * @brief Error code map for errno.
 * @see https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
 */
#define INFRA_SYS_ERRNO_POSIX_MAP(xx)           \
    xx(ETIMEDOUT,           INFRA_ETIMEDOUT)    \
    xx(ENOENT,              INFRA_ENOENT)

typedef struct infra_errno_map
{
    int         src;
    int         dst;
} infra_errno_map_t;

typedef struct infra_strerror_map
{
    int         src;
    const char* dst;
} infra_strerror_map_t;

#if defined(INFRA_NEED_FOPEN_S)
int infra_fopen_s(FILE** pFile, const char* filename, const char* mode)
{
    if ((*pFile = fopen(filename, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}
#endif

int infra_translate_sys_error(int errcode)
{
#define EXPAND_CODE_AS_ERRNO_MAP(a, b)  { a, b },

    static infra_errno_map_t s_errno_map[] = {
#if defined(_WIN32)
        INFRA_SYS_ERRNO_WIN32_MAP(EXPAND_CODE_AS_ERRNO_MAP)
#endif
        INFRA_SYS_ERRNO_POSIX_MAP(EXPAND_CODE_AS_ERRNO_MAP)

        { 0, 0 }, /* 0 is always success */
    };

    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_errno_map); i++)
    {
        if (errcode == s_errno_map[i].src)
        {
            return s_errno_map[i].dst;
        }
    }

    abort();

#undef EXPAND_CODE_AS_ERRNO_MAP
}

const char* infra_strerror(int errcode)
{
    static infra_strerror_map_t s_errorstr_map[] = {
        { INFRA_EOF,        "End of file" },
        { INFRA_UNKNOWN,    "Unknown error" },
        { INFRA_ETIMEDOUT,  "Connection timed out" },
        { INFRA_ENOENT,     "No such file or directory" },
        { 0,                "Operation success" },
    };

    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_errorstr_map); i++)
    {
        if (s_errorstr_map[i].src == errcode)
        {
            return s_errorstr_map[i].dst;
        }
    }

    abort();
}
