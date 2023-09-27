#ifndef __INFRA_UTILS_EXEC_H__
#define __INFRA_UTILS_EXEC_H__

#if defined(_WIN32)

#include <Windows.h>
typedef HANDLE  infra_os_pid_t;
#define INFRA_OS_PID_INVALID    INVALID_HANDLE_VALUE

#else

#include <sys/types.h>
typedef pid_t   infra_os_pid_t;
#define INFRA_OS_PID_INVALID    -1

#endif

#include <stdint.h>
#include "pipe.h"

#define INFRA_OS_WAITPID_INFINITE   ((uint32_t)-1)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct infra_exec_opt
{
    char*           cwd;        /**< Current working directory. */
    char**          args;       /**< Command line arguments. */
    char**          envs;       /**< Environments. */

    infra_os_pipe_t h_stdin;    /**< pipe for stdin. */
    infra_os_pipe_t h_stdout;   /**< pipe for stdout. */
    infra_os_pipe_t h_stderr;   /**< pipe for stderr. */
} infra_exec_opt_t;

/**
 * @brief Initializes the process handle and starts the process.
 * @param[out] pid  Process ID.
 * @param[in] file  Command to execute.
 * @param[in] opt   Process options.
 * @return          0 if success, otherwise failed.
 */
API_LOCAL int infra_exec(infra_os_pid_t* pid, const char* file, infra_exec_opt_t* opt);

/**
 * @brief Wait for process to end.
 * @param[in] pid   Process ID.
 * @param[in] ms    Timeout in milliseconds. #INFRA_OS_WAITPID_INFINITE for wait infinite.
 * @return          0 if process terminal, or following error code:
 *                  INFRA_ETIMEDOUT: The time-out interval elapsed.
 */
API_LOCAL int infra_waitpid(infra_os_pid_t pid, int* code, uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
