#include "pipe.h"
#include "function/__init__.h"

#if defined(_WIN32)

#include <stdio.h>

int infra_pipe_open(infra_os_pipe_t fd[2], int r_flags, int w_flags)
{
    DWORD mode;
    char buf[1024];
    int errcode;
    static long volatile s_pipe_serial_no = 0;

    snprintf(buf, sizeof(buf), "\\\\.\\Pipe\\RemoteExeAnon.%08lx.%08lx",
        (long)GetCurrentProcessId(), InterlockedIncrement(&s_pipe_serial_no));

    fd[0] = INVALID_HANDLE_VALUE;
    fd[1] = INVALID_HANDLE_VALUE;

    mode = (r_flags & INFRA_OS_PIPE_NONBLOCK) ? (PIPE_TYPE_BYTE | PIPE_NOWAIT) : (PIPE_TYPE_BYTE | PIPE_WAIT);
    fd[0] = CreateNamedPipeA(buf, PIPE_ACCESS_INBOUND, mode, 1, 4096, 4096, 0, NULL);
    if (fd[0] == INVALID_HANDLE_VALUE)
    {
        errcode = GetLastError();
        goto error_generic;
    }

    fd[1] = CreateFileA(buf, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd[1] == INVALID_HANDLE_VALUE)
    {
        errcode = GetLastError();
        goto error_generic;
    }

    if (w_flags & INFRA_OS_PIPE_NONBLOCK)
    {
        mode = PIPE_NOWAIT;
        if (SetNamedPipeHandleState(fd[1], &mode, NULL, NULL) == 0)
        {
            errcode = GetLastError();
            goto error_generic;
        }
    }

    mode = (r_flags & INFRA_OS_PIPE_CLOEXEC) ? 0 : HANDLE_FLAG_INHERIT;
    if (!SetHandleInformation(fd[0], HANDLE_FLAG_INHERIT, mode))
    {
        errcode = GetLastError();
        goto error_generic;
    }

    mode = (w_flags & INFRA_OS_PIPE_CLOEXEC) ? 0 : HANDLE_FLAG_INHERIT;
    if (!SetHandleInformation(fd[1], HANDLE_FLAG_INHERIT, mode))
    {
        errcode = GetLastError();
        goto error_generic;
    }

    return 0;

error_generic:
    if (fd[0] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(fd[0]); fd[0] = INVALID_HANDLE_VALUE;
    }
    if (fd[1] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(fd[1]); fd[1] = INVALID_HANDLE_VALUE;
    }
    return infra_translate_sys_error(errcode);
}

void infra_pipe_close(infra_os_pipe_t fd)
{
    CloseHandle(fd);
}

ssize_t infra_pipe_write(infra_os_pipe_t fd, const void* data, size_t size)
{
    DWORD write_sz;
    if (WriteFile(fd, data, (DWORD)size, &write_sz, NULL))
    {
        return write_sz;
    }

    DWORD errcode = GetLastError();
    return infra_translate_sys_error(errcode);
}

ssize_t infra_pipe_read(infra_os_pipe_t fd, void* buf, size_t buf_sz)
{
    DWORD read_sz;
    if (ReadFile(fd, buf, (DWORD)buf_sz, &read_sz, NULL))
    {
        return read_sz;
    }

    DWORD errcode = GetLastError();
    if (errcode == ERROR_NO_DATA)
    {
        return 0;
    }

    return infra_translate_sys_error(errcode);
}

#else

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(_AIX) || \
    defined(__APPLE__) || \
    defined(__DragonFly__) || \
    defined(__FreeBSD__) || \
    defined(__FreeBSD_kernel__) || \
    defined(__linux__) || \
    defined(__OpenBSD__) || \
    defined(__NetBSD__)
#define INFRA_USE_IOCTL

#else

static int ev__fcntl_getfd_unix(int fd)
{
    int flags;

    do
    {
        flags = fcntl(fd, F_GETFD);
    } while (flags == -1 && errno == EINTR);

    return flags;
}

static int ev__fcntl_getfl_unix(int fd)
{
    int mode;
    do
    {
        mode = fcntl(fd, F_GETFL);
    } while (mode == -1 && errno == EINTR);
    return mode;
}

#endif

static int ev__cloexec(int fd, int set)
{
#if defined(INFRA_USE_IOCTL)
    int r;

    do
    {
        r = ioctl(fd, set ? FIOCLEX : FIONCLEX);
    } while (r == -1 && errno == EINTR);

    if (r)
    {
        return errno;
    }

    return 0;

#else

    int flags;

    int r = ev__fcntl_getfd_unix(fd);
    if (r == -1)
    {
        return errno;
    }

    /* Bail out now if already set/clear. */
    if (!!(r & FD_CLOEXEC) == !!set)
    {
        return 0;
    }

    if (set)
    {
        flags = r | FD_CLOEXEC;
    }
    else
    {
        flags = r & ~FD_CLOEXEC;
    }

    do
    {
        r = fcntl(fd, F_SETFD, flags);
    } while (r == -1 && errno == EINTR);

    if (r)
    {
        return errno;
    }

    return 0;

#endif
}

static int ev__nonblock(int fd, int set)
{
#if defined(INFRA_USE_IOCTL)

    int r;

    do
    {
        r = ioctl(fd, FIONBIO, &set);
    } while (r == -1 && errno == EINTR);

    if (r)
    {
        return errno;
    }

    return 0;

#else

    int flags;

    int r = ev__fcntl_getfl_unix(fd);
    if (r == -1)
    {
        return errno;
    }

    /* Bail out now if already set/clear. */
    if (!!(r & O_NONBLOCK) == !!set)
    {
        return 0;
    }

    if (set)
    {
        flags = r | O_NONBLOCK;
    }
    else
    {
        flags = r & ~O_NONBLOCK;
    }

    do
    {
        r = fcntl(fd, F_SETFL, flags);
    } while (r == -1 && errno == EINTR);

    if (r)
    {
        return errno;
    }

    return 0;

#endif
}

int infra_pipe_open(infra_os_pipe_t fd[2], int r_flags, int w_flags)
{
    int ret;

    fd[0] = INFRA_OS_PIPE_INVALID;
    fd[1] = INFRA_OS_PIPE_INVALID;

    if (pipe(fd) != 0)
    {
        return errno;
    }

    if ((ret = ev__nonblock(fd[0], r_flags & INFRA_OS_PIPE_NONBLOCK)) != 0)
    {
        goto failure;
    }
    if ((ret = ev__nonblock(fd[1], w_flags & INFRA_OS_PIPE_NONBLOCK)) != 0)
    {
        goto failure;
    }

    if ((ret = ev__cloexec(fd[0], r_flags & INFRA_OS_PIPE_CLOEXEC)) != 0)
    {
        goto failure;
    }
    if ((ret = ev__cloexec(fd[1], w_flags & INFRA_OS_PIPE_CLOEXEC)) != 0)
    {
        goto failure;
    }

    return 0;

failure:
    infra_pipe_close(fd[0]); fd[0] = INFRA_OS_PIPE_INVALID;
    infra_pipe_close(fd[1]); fd[1] = INFRA_OS_PIPE_INVALID;
    return ret;
}

void infra_pipe_close(infra_os_pipe_t fd)
{
    close(fd);
}

ssize_t infra_pipe_write(infra_os_pipe_t fd, const void* data, size_t size)
{
    ssize_t write_sz = write(fd, data, size);
    if (write_sz >= 0)
    {
        return write_sz;
    }

    int errcode = errno;
    if (errcode == EAGAIN
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
        || errcode == EWOULDBLOCK
#endif
        )
    {
        return 0;
    }

    return infra_translate_sys_error(errcode);
}

ssize_t infra_pipe_read(infra_os_pipe_t fd, void* buf, size_t buf_sz)
{
    ssize_t read_sz;
    do
    {
        read_sz = read(fd, buf, buf_sz);
    } while (read_sz == -1 && errno == EINTR);

    if (read_sz > 0)
    {
        return read_sz;
    }
    else if (read_sz == 0)
    {
        return INFRA_EOF;
    }

    int errcode = errno;
    if (errcode == EAGAIN
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
        || errcode == EWOULDBLOCK
#endif
        )
    {
        return 0;
    }

    return infra_translate_sys_error(errcode);
}

#endif
