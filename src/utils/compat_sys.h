#ifndef __INFRA_LUA_COMPAT_SYS_H__
#define __INFRA_LUA_COMPAT_SYS_H__

#include <stdio.h>
#include <errno.h>
#include "defs.h"

/**
 * @brief Error code.
 * @{
 */
#define INFRA_EOF       (-4095)
#define INFRA_UNKNOWN   (-4094)
#define INFRA_ETIMEDOUT (-138)
#define INFRA_ENOENT    (-2)
/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)

#include <Windows.h>
#define strtok_r(str, delim, saveptr)   strtok_s(str, delim, saveptr)
#define strerror_r(errcode, buf, len)   strerror_s(buf,len, errcode)
#define strdup(s)                       _strdup(s)
#define strcasecmp(s1, s2)              _stricmp(s1, s2)
#define sscanf(b, f, ...)               sscanf_s(b, f, ##__VA_ARGS__)

#else

#define INFRA_NEED_FOPEN_S
#define fopen_s(p, f, m)                infra_fopen_s(p, f, m)

/**
 * @brief Opens a file.
 * @see fopen()
 * @param[out] pFile    A pointer to the file pointer that will receive the
 *   pointer to the opened file.
 * @param[in] filename  Filename.
 * @param[in] mode      Type of access permitted.
 * @return              Zero if successful; an error code on failure.
 */
API_LOCAL int infra_fopen_s(FILE** pFile, const char* filename, const char* mode);

#endif

/**
 * @brief Translate system error to standard error.
 * @param[in] errcode   For windows, it must comes from GetLastError(). For
 *                      unix, it must comes from errno.
 */
API_LOCAL int infra_translate_sys_error(int errcode);

/**
 * @brief Convert error code into string that describes the error code.
 * @param[in] errcode   Error code.
 * @return              Describe string.
 */
API_LOCAL const char* infra_strerror(int errcode);

#ifdef __cplusplus
}
#endif
#endif
