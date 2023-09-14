#ifndef __INFRA_UTILS_PIPE_H__
#define __INFRA_UTILS_PIPE_H__

#if defined(_WIN32)

#include <Windows.h>
typedef HANDLE  infra_os_pipe_t;
#define INFRA_OS_PIPE_INVALID   INVALID_HANDLE_VALUE

#else

typedef int     infra_os_pipe_t;
#define INFRA_OS_PIPE_INVALID   -1

#endif

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum infra_os_pipe_flag
{
    INFRA_OS_PIPE_NONBLOCK  = 0x01,
    INFRA_OS_PIPE_CLOEXEC   = 0x02,
} infra_os_pipe_flag_t;

/**
 * @brief Creates a unidirectional data channel that can be used for
 *   interprocess communication.
 * @param[out] fd       Channel. fd[0] for read, fd[1] for write.
 * @param[in] r_flags   Flags for fd[0].
 * @param[in] w_flags   Flags for fd[1].
 * @return              0 if success, otherwise failed.
 */
int infra_pipe_open(infra_os_pipe_t fd[2], int r_flags, int w_flags);

/**
 * @brief Close channel.
 * @param[in] fd        Channel.
 */
void infra_pipe_close(infra_os_pipe_t fd);

/**
 * @brief Write data into pipe.
 * @param[in] fd        Channel.
 * @param[in] data      Data.
 * @param[in] size      Data size.
 * @return              Less than 0 if failure, otherwise success.
 */
ssize_t infra_pipe_write(infra_os_pipe_t fd, const void* data, size_t size);

/**
 * @brief Read data from pipe.
 * @param[in] fd        Channel.
 * @param[out] buf      Buffer.
 * @param[in] buf_sz    Buffer size.
 * @return              The number of bytes received, or less than 0 if failure.
 */
ssize_t infra_pipe_read(infra_os_pipe_t fd, void* buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif
