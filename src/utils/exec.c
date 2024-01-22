#include "exec.h"
#include "function/__init__.h"
#include <assert.h>

#if defined(_WIN32)

#include <malloc.h>
#include <wchar.h>

typedef struct infra_exec_data
{
    WCHAR*              w_app;
    WCHAR*              w_app_path;
    WCHAR*              w_cwd;
    WCHAR*              w_args;
    WCHAR*              w_envs;
    WCHAR*              w_path;
} infra_exec_data_t;

typedef struct env_var
{
    const WCHAR* const  wide;
    const WCHAR* const  wide_eq;
    const size_t        len;        /**< including null or '=' */
} env_var_t;

#define E_V(str) { L##str, L##str L"=", sizeof(str) }

static const env_var_t required_vars[] = { /* keep me sorted */
    E_V("HOMEDRIVE"),
    E_V("HOMEPATH"),
    E_V("LOGONSERVER"),
    E_V("PATH"),
    E_V("SYSTEMDRIVE"),
    E_V("SYSTEMROOT"),
    E_V("TEMP"),
    E_V("USERDOMAIN"),
    E_V("USERNAME"),
    E_V("USERPROFILE"),
    E_V("WINDIR"),
};

static int _find_path(WCHAR* env, WCHAR** dst)
{
    for (; env != NULL && *env != 0; env += wcslen(env) + 1)
    {
        if ((env[0] == L'P' || env[0] == L'p') &&
            (env[1] == L'A' || env[1] == L'a') &&
            (env[2] == L'T' || env[2] == L't') &&
            (env[3] == L'H' || env[3] == L'h') &&
            (env[4] == L'='))
        {
            if ((*dst = _wcsdup(&env[5])) == NULL)
            {
                return ERROR_OUTOFMEMORY;
            }
            return 0;
        }
    }

    *dst = NULL;
    return 0;
}

/*
 * Helper function for search_path
 */
static WCHAR* search_path_join_test(const WCHAR* dir,
    size_t dir_len,
    const WCHAR* name,
    size_t name_len,
    const WCHAR* ext,
    size_t ext_len,
    const WCHAR* cwd,
    size_t cwd_len)
{
    WCHAR* result, * result_pos;
    DWORD attrs;
    if (dir_len > 2 &&
        ((dir[0] == L'\\' || dir[0] == L'/') &&
            (dir[1] == L'\\' || dir[1] == L'/')))
    {
        /* It's a UNC path so ignore cwd */
        cwd_len = 0;
    }
    else if (dir_len >= 1 && (dir[0] == L'/' || dir[0] == L'\\'))
    {
        /* It's a full path without drive letter, use cwd's drive letter only */
        cwd_len = 2;
    }
    else if (dir_len >= 2 && dir[1] == L':' &&
        (dir_len < 3 || (dir[2] != L'/' && dir[2] != L'\\')))
    {
        /* It's a relative path with drive letter (ext.g. D:../some/file)
         * Replace drive letter in dir by full cwd if it points to the same drive,
         * otherwise use the dir only.
         */
        if (cwd_len < 2 || _wcsnicmp(cwd, dir, 2) != 0)
        {
            cwd_len = 0;
        }
        else
        {
            dir += 2;
            dir_len -= 2;
        }
    }
    else if (dir_len > 2 && dir[1] == L':')
    {
        /* It's an absolute path with drive letter
         * Don't use the cwd at all
         */
        cwd_len = 0;
    }

    /* Allocate buffer for output */
    size_t total_sz = cwd_len + 1 + dir_len + 1 + name_len + 1 + ext_len + 1;
    result = result_pos = (WCHAR*)malloc(sizeof(WCHAR) * total_sz);
    if (result_pos == NULL)
    {
        abort();
    }

    /* Copy cwd */
    wcsncpy_s(result_pos, total_sz, cwd, cwd_len);
    result_pos += cwd_len;

    /* Add a path separator if cwd didn't end with one */
    if (cwd_len && wcsrchr(L"\\/:", result_pos[-1]) == NULL)
    {
        result_pos[0] = L'\\';
        result_pos++;
    }

    /* Copy dir */
    wcsncpy_s(result_pos, result + total_sz - result_pos, dir, dir_len);
    result_pos += dir_len;

    /* Add a separator if the dir didn't end with one */
    if (dir_len && wcsrchr(L"\\/:", result_pos[-1]) == NULL)
    {
        result_pos[0] = L'\\';
        result_pos++;
    }

    /* Copy filename */
    wcsncpy_s(result_pos, result + total_sz - result_pos, name, name_len);
    result_pos += name_len;

    if (ext_len)
    {
        /* Add a dot if the filename didn't end with one */
        if (name_len && result_pos[-1] != '.')
        {
            result_pos[0] = L'.';
            result_pos++;
        }

        /* Copy extension */
        wcsncpy_s(result_pos, result + total_sz - result_pos, ext, ext_len);
        result_pos += ext_len;
    }

    /* Null terminator */
    result_pos[0] = L'\0';

    attrs = GetFileAttributesW(result);

    if (attrs != INVALID_FILE_ATTRIBUTES &&
        !(attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        return result;
    }

    free(result);
    return NULL;
}

/*
 * Helper function for search_path
 */
static WCHAR* path_search_walk_ext(const WCHAR* dir,
    size_t dir_len,
    const WCHAR* name,
    size_t name_len,
    WCHAR* cwd,
    size_t cwd_len,
    int name_has_ext)
{
    WCHAR* result;

    /* If the name itself has a nonempty extension, try this extension first */
    if (name_has_ext)
    {
        result = search_path_join_test(dir, dir_len,
            name, name_len,
            L"", 0,
            cwd, cwd_len);
        if (result != NULL)
        {
            return result;
        }
    }

    /* Try .com extension */
    result = search_path_join_test(dir, dir_len,
        name, name_len,
        L"com", 3,
        cwd, cwd_len);
    if (result != NULL)
    {
        return result;
    }

    /* Try .exe extension */
    result = search_path_join_test(dir, dir_len,
        name, name_len,
        L"exe", 3,
        cwd, cwd_len);
    if (result != NULL)
    {
        return result;
    }

    return NULL;
}

static int _search_path(const WCHAR* file, WCHAR* cwd, const WCHAR* path, WCHAR** dst)
{
    int file_has_dir;
    WCHAR* result = NULL;
    WCHAR* file_name_start;
    WCHAR* dot;
    const WCHAR* dir_start, * dir_end, * dir_path;
    size_t dir_len;
    int name_has_ext;

    size_t file_len = wcslen(file);
    size_t cwd_len = wcslen(cwd);

    /* If the caller supplies an empty filename,
     * we're not gonna return c:\windows\.exe -- GFY!
     */
    if (file_len == 0 || (file_len == 1 && file[0] == L'.'))
    {
        *dst = NULL;
        return 0;
    }

    /* Find the start of the filename so we can split the directory from the
     * name. */
    for (file_name_start = (WCHAR*)file + file_len;
        file_name_start > file
        && file_name_start[-1] != L'\\'
        && file_name_start[-1] != L'/'
        && file_name_start[-1] != L':';
        file_name_start--);

    file_has_dir = file_name_start != file;

    /* Check if the filename includes an extension */
    dot = wcschr(file_name_start, L'.');
    name_has_ext = (dot != NULL && dot[1] != L'\0');

    if (file_has_dir)
    {
        /* The file has a path inside, don't use path */
        result = path_search_walk_ext(
            file, file_name_start - file,
            file_name_start, file_len - (file_name_start - file),
            cwd, cwd_len,
            name_has_ext);
    }
    else
    {
        dir_end = path;

        /* The file is really only a name; look in cwd first, then scan path */
        result = path_search_walk_ext(L"", 0,
            file, file_len,
            cwd, cwd_len,
            name_has_ext);

        while (result == NULL)
        {
            if (*dir_end == L'\0')
            {
                break;
            }

            /* Skip the separator that dir_end now points to */
            if (dir_end != path || *path == L';')
            {
                dir_end++;
            }

            /* Next slice starts just after where the previous one ended */
            dir_start = dir_end;

            /* If path is quoted, find quote end */
            if (*dir_start == L'"' || *dir_start == L'\'')
            {
                dir_end = wcschr(dir_start + 1, *dir_start);
                if (dir_end == NULL)
                {
                    dir_end = wcschr(dir_start, L'\0');
                }
            }
            /* Slice until the next ; or \0 is found */
            dir_end = wcschr(dir_end, L';');
            if (dir_end == NULL)
            {
                dir_end = wcschr(dir_start, L'\0');
            }

            /* If the slice is zero-length, don't bother */
            if (dir_end - dir_start == 0)
            {
                continue;
            }

            dir_path = dir_start;
            dir_len = dir_end - dir_start;

            /* Adjust if the path is quoted. */
            if (dir_path[0] == '"' || dir_path[0] == '\'')
            {
                ++dir_path;
                --dir_len;
            }

            if (dir_path[dir_len - 1] == '"' || dir_path[dir_len - 1] == '\'')
            {
                --dir_len;
            }

            result = path_search_walk_ext(dir_path, dir_len,
                file, file_len,
                cwd, cwd_len,
                name_has_ext);
        }
    }

    if ((*dst = _wcsdup(result)) == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    return 0;
}

/*
 * Quotes command line arguments
 * Returns a pointer to the end (next char to be written) of the buffer
 */
static WCHAR* quote_cmd_arg(const WCHAR* source, WCHAR* target, size_t target_sz)
{
    size_t len = wcslen(source);
    size_t i;
    int quote_hit;
    WCHAR* start;

    if (len == 0)
    {
        /* Need double quotation for empty argument */
        *(target++) = L'"';
        *(target++) = L'"';
        return target;
    }

    if (NULL == wcspbrk(source, L" \t\""))
    {
        /* No quotation needed */
        wcsncpy_s(target, target_sz, source, len);
        target += len;
        return target;
    }

    if (NULL == wcspbrk(source, L"\"\\"))
    {
        /*
         * No embedded double quotes or backlashes, so I can just wrap
         * quote marks around the whole thing.
         */
        *(target++) = L'"';
        wcsncpy_s(target, target_sz - 1, source, len);
        target += len;
        *(target++) = L'"';
        return target;
    }

    /*
     * Expected input/output:
     *   input : hello"world
     *   output: "hello\"world"
     *   input : hello""world
     *   output: "hello\"\"world"
     *   input : hello\world
     *   output: hello\world
     *   input : hello\\world
     *   output: hello\\world
     *   input : hello\"world
     *   output: "hello\\\"world"
     *   input : hello\\"world
     *   output: "hello\\\\\"world"
     *   input : hello world\
     *   output: "hello world\\"
     */

    * (target++) = L'"';
    start = target;
    quote_hit = 1;

    for (i = len; i > 0; --i)
    {
        *(target++) = source[i - 1];

        if (quote_hit && source[i - 1] == L'\\')
        {
            *(target++) = L'\\';
        }
        else if (source[i - 1] == L'"')
        {
            quote_hit = 1;
            *(target++) = L'\\';
        }
        else
        {
            quote_hit = 0;
        }
    }
    target[0] = L'\0';
    _wcsrev(start);
    *(target++) = L'"';
    return target;
}

static int make_program_args(char** args, WCHAR** dst_ptr)
{
    char** arg;
    WCHAR* dst = NULL;
    WCHAR* temp_buffer = NULL;
    size_t dst_len = 0;
    size_t temp_buffer_len = 0;
    WCHAR* pos;
    int arg_count = 0;
    int err = 0;

    /* Count the required size. */
    for (arg = args; *arg; arg++)
    {
        DWORD arg_len;

        arg_len = MultiByteToWideChar(CP_UTF8,
            0,
            *arg,
            -1,
            NULL,
            0);
        if (arg_len == 0)
        {
            return GetLastError();
        }

        dst_len += arg_len;

        if (arg_len > temp_buffer_len)
            temp_buffer_len = arg_len;

        arg_count++;
    }

    /* Adjust for potential quotes. Also assume the worst-case scenario that
     * every character needs escaping, so we need twice as much space. */
    dst_len = dst_len * 2 + arg_count * 2;

    /* Allocate buffer for the final command line. */
    dst = (WCHAR*)malloc(dst_len * sizeof(WCHAR));
    if (dst == NULL)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }

    /* Allocate temporary working buffer. */
    temp_buffer = (WCHAR*)malloc(temp_buffer_len * sizeof(WCHAR));
    if (temp_buffer == NULL)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }

    pos = dst;
    for (arg = args; *arg; arg++)
    {
        DWORD arg_len;

        /* Convert argument to wide char. */
        arg_len = MultiByteToWideChar(CP_UTF8,
            0,
            *arg,
            -1,
            temp_buffer,
            (int)(dst + dst_len - pos));
        if (arg_len == 0)
        {
            err = GetLastError();
            goto error;
        }


        /* Quote/escape, if needed. */
        pos = quote_cmd_arg(temp_buffer, pos, dst + dst_len - pos);

        *pos++ = *(arg + 1) ? L' ' : L'\0';
    }

    free(temp_buffer);

    *dst_ptr = dst;
    return 0;

error:
    free(dst);
    free(temp_buffer);
    return err;
}

static int env_strncmp(const wchar_t* a, int na, const wchar_t* b)
{
    wchar_t* a_eq;
    wchar_t* b_eq;
    wchar_t* A = NULL;
    wchar_t* B = NULL;
    int nb;
    int r;

    if (na < 0)
    {
        a_eq = wcschr(a, L'=');
        assert(a_eq);
        na = (int)(long)(a_eq - a);
    }
    else
    {
        na--;
    }
    b_eq = wcschr(b, L'=');
    assert(b_eq);
    nb = (int)(b_eq - b);

    if ((A = malloc((na + 1) * sizeof(wchar_t))) == NULL)
    {
        abort();
    }
    if ((B = malloc((nb + 1) * sizeof(wchar_t))) == NULL)
    {
        abort();
    }

    r = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, a, na, A, na);
    assert(r == na);
    A[na] = L'\0';
    r = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, b, nb, B, nb);
    assert(r == nb);
    B[nb] = L'\0';

    for (;;)
    {
        wchar_t AA = *A++;
        wchar_t BB = *B++;
        if (AA < BB)
        {
            r = -1;
            goto finish;
        }
        else if (AA > BB)
        {
            r = 1;
            goto finish;
        }
        else if (!AA && !BB)
        {
            r = 0;
            goto finish;
        }
    }

finish:
    free(A); A = NULL;
    free(B); B = NULL;
    return r;
}

static int qsort_wcscmp(const void* a, const void* b) {
    wchar_t* astr = *(wchar_t* const*)a;
    wchar_t* bstr = *(wchar_t* const*)b;
    return env_strncmp(astr, -1, bstr);
}

/*
 * The way windows takes environment variables is different than what C does;
 * Windows wants a contiguous block of null-terminated strings, terminated
 * with an additional null.
 *
 * Windows has a few "essential" environment variables. winsock will fail
 * to initialize if SYSTEMROOT is not defined; some APIs make reference to
 * TEMP. SYSTEMDRIVE is probably also important. We therefore ensure that
 * these get defined if the input environment block does not contain any
 * values for them.
 *
 * Also add variables known to Cygwin to be required for correct
 * subprocess operation in many cases:
 * https://github.com/Alexpux/Cygwin/blob/b266b04fbbd3a595f02ea149e4306d3ab9b1fe3d/winsup/cygwin/environ.cc#L955
 *
 */
static int make_program_env(char* env_block[], WCHAR** dst_ptr)
{
    WCHAR* dst;
    WCHAR* ptr;
    char** env;
    size_t env_len = 0;
    int len;
    size_t i;
    DWORD var_size;
    size_t env_block_count = 1; /* 1 for null-terminator */
    WCHAR* dst_copy;
    WCHAR** ptr_copy;
    WCHAR** env_copy;
    DWORD required_vars_value_len[ARRAY_SIZE(required_vars)];

    /* first pass: determine size in UTF-16 */
    for (env = env_block; *env; env++)
    {
        if (strchr(*env, '='))
        {
            len = MultiByteToWideChar(CP_UTF8,
                0,
                *env,
                -1,
                NULL,
                0);
            if (len <= 0)
            {
                return GetLastError();
            }
            env_len += len;
            env_block_count++;
        }
    }

    /* second pass: copy to UTF-16 environment block */
    dst_copy = (WCHAR*)malloc(env_len * sizeof(WCHAR));
    if (dst_copy == NULL && env_len > 0)
    {
        return ERROR_OUTOFMEMORY;
    }
    env_copy = _malloca(env_block_count * sizeof(WCHAR*));

    ptr = dst_copy;
    ptr_copy = env_copy;
    for (env = env_block; *env; env++)
    {
        if (strchr(*env, '='))
        {
            len = MultiByteToWideChar(CP_UTF8,
                0,
                *env,
                -1,
                ptr,
                (int)(env_len - (ptr - dst_copy)));
            if (len <= 0)
            {
                DWORD err = GetLastError();
                free(dst_copy);
                return err;
            }
            *ptr_copy++ = ptr;
            ptr += len;
        }
    }
    *ptr_copy = NULL;
    assert(env_len == 0 || env_len == (size_t)(ptr - dst_copy));

    /* sort our (UTF-16) copy */
    qsort(env_copy, env_block_count - 1, sizeof(wchar_t*), qsort_wcscmp);

    /* third pass: check for required variables */
    for (ptr_copy = env_copy, i = 0; i < ARRAY_SIZE(required_vars); )
    {
        int cmp;
        if (!*ptr_copy) {
            cmp = -1;
        }
        else
        {
            cmp = env_strncmp(required_vars[i].wide_eq,
                (int)required_vars[i].len,
                *ptr_copy);
        }
        if (cmp < 0)
        {
            /* missing required var */
            var_size = GetEnvironmentVariableW(required_vars[i].wide, NULL, 0);
            required_vars_value_len[i] = var_size;
            if (var_size != 0)
            {
                env_len += required_vars[i].len;
                env_len += var_size;
            }
            i++;
        }
        else
        {
            ptr_copy++;
            if (cmp == 0)
            {
                i++;
            }
        }
    }

    /* final pass: copy, in sort order, and inserting required variables */
    dst = malloc((1 + env_len) * sizeof(WCHAR));
    if (!dst)
    {
        free(dst_copy);
        return ERROR_OUTOFMEMORY;
    }

    for (ptr = dst, ptr_copy = env_copy, i = 0;
        *ptr_copy || i < ARRAY_SIZE(required_vars);
        ptr += len)
    {
        int cmp;
        if (i >= ARRAY_SIZE(required_vars))
        {
            cmp = 1;
        }
        else if (!*ptr_copy)
        {
            cmp = -1;
        }
        else
        {
            cmp = env_strncmp(required_vars[i].wide_eq,
                (int)required_vars[i].len,
                *ptr_copy);
        }
        if (cmp < 0)
        {
            /* missing required var */
            len = required_vars_value_len[i];
            if (len)
            {
                wcscpy_s(ptr, dst + env_len + 1 - ptr, required_vars[i].wide_eq);
                ptr += required_vars[i].len;
                var_size = GetEnvironmentVariableW(required_vars[i].wide,
                    ptr,
                    (int)(env_len - (ptr - dst)));
                if (var_size != (DWORD)(len - 1))
                { /* TODO: handle race condition? */
                    fprintf(stderr, "GetEnvironmentVariableW\n");
                    abort();
                }
            }
            i++;
        }
        else
        {
            /* copy var from env_block */
            len = (int)wcslen(*ptr_copy) + 1;
            wmemcpy(ptr, *ptr_copy, len);
            ptr_copy++;
            if (cmp == 0)
            {
                i++;
            }
        }
    }

    /* Terminate with an extra NULL. */
    assert(env_len == (size_t)(ptr - dst));
    *ptr = L'\0';

    free(dst_copy);
    *dst_ptr = dst;
    return 0;
}

static int _infra_exec(infra_os_pid_t* pid, infra_exec_opt_t* opt, infra_exec_data_t* data)
{
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    STARTUPINFOW siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(siStartInfo);
    if (opt->h_stdin != INFRA_OS_PIPE_INVALID)
    {
        siStartInfo.hStdInput = opt->h_stdin;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    }
    if (opt->h_stdout != INFRA_OS_PIPE_INVALID)
    {
        siStartInfo.hStdOutput = opt->h_stdout;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    }
    if (opt->h_stderr != INFRA_OS_PIPE_INVALID)
    {
        siStartInfo.hStdError = opt->h_stderr;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    BOOL cp_ret = CreateProcessW(data->w_app_path,
        data->w_args,
        NULL,
        NULL,
        TRUE,
        CREATE_UNICODE_ENVIRONMENT,
        data->w_envs,
        data->w_cwd,
        &siStartInfo,
        &piProcInfo);
    if (!cp_ret)
    {
        return GetLastError();
    }
    CloseHandle(piProcInfo.hThread);
    *pid = piProcInfo.hProcess;

    return 0;
}

int infra_exec(infra_os_pid_t* pid, const char* file, infra_exec_opt_t* opt)
{
    int errcode = 0;

    infra_exec_data_t data;
    memset(&data, 0, sizeof(data));

    if ((data.w_app = infra_utf8_to_wide(file)) == NULL)
    {
        errcode = ERROR_OUTOFMEMORY;
        goto finish;
    }

    if (opt != NULL && opt->cwd != NULL)
    {
        if ((data.w_cwd = infra_utf8_to_wide(opt->cwd)) == NULL)
        {
            errcode = ERROR_OUTOFMEMORY;
            goto finish;
        }
    }
    else
    {
        DWORD len = GetCurrentDirectoryW(0, NULL);
        if (len == 0)
        {
            errcode = GetLastError();
            goto finish;
        }
        if ((data.w_cwd = malloc(sizeof(WCHAR) * len)) == NULL)
        {
            errcode = ERROR_OUTOFMEMORY;
            goto finish;
        }

        DWORD r = GetCurrentDirectoryW(len, data.w_cwd);
        if (r == 0 || r >= len)
        {
            errcode = GetLastError();
            goto finish;
        }
    }

    if ((errcode = _find_path(data.w_envs, &data.w_path)) != 0)
    {
        goto finish;
    }
    if (data.w_path == NULL)
    {
        DWORD path_len = GetEnvironmentVariableW(L"PATH", NULL, 0);
        if (path_len == 0)
        {
            errcode = GetLastError();
            goto finish;
        }

        if ((data.w_path = malloc(sizeof(WCHAR) * path_len)) == NULL)
        {
            errcode = ERROR_OUTOFMEMORY;
            goto finish;
        }

        DWORD r = GetEnvironmentVariableW(L"PATH", data.w_path, path_len);
        if (r == 0 || r >= path_len)
        {
            errcode = GetLastError();
            goto finish;
        }
    }

    if ((errcode = _search_path(data.w_app, data.w_cwd, data.w_path, &data.w_app_path)) != 0)
    {
        goto finish;
    }
    if (data.w_app_path == NULL)
    {
        errcode = ERROR_FILE_NOT_FOUND;
        goto finish;
    }

    char** args = { NULL };
    if (opt != NULL && opt->args != NULL)
    {
        args = opt->args;
    }
    if ((errcode = make_program_args(args, &data.w_args)) != 0)
    {
        goto finish;
    }

    if (opt != NULL && opt->envs != NULL)
    {
        if ((errcode = make_program_env(opt->envs, &data.w_envs)) != 0)
        {
            goto finish;
        }
    }

    errcode = _infra_exec(pid, opt, &data);

finish:
    free(data.w_app);
    free(data.w_app_path);
    free(data.w_cwd);
    free(data.w_args);
    free(data.w_envs);
    free(data.w_path);
    return infra_translate_sys_error(errcode);
}

int infra_waitpid(infra_os_pid_t pid, int* code, uint32_t ms)
{
    DWORD ret, errcode;
    DWORD dwMilliseconds = ms;
    if (ms == INFRA_OS_WAITPID_INFINITE)
    {
        dwMilliseconds = INFINITE;
    }
    
    switch (ret = WaitForSingleObject(pid, ms))
    {
    case WAIT_ABANDONED:
        break;

    case WAIT_OBJECT_0: {
            DWORD exit_code;
            if (!GetExitCodeProcess(pid, &exit_code))
            {
                abort();
            }
            if (exit_code == STILL_ACTIVE)
            {
                abort();
            }
            if (code != NULL)
            {
                *code = exit_code;
            }
            CloseHandle(pid);
        }
        return 0;

    case WAIT_TIMEOUT:
        return INFRA_ETIMEDOUT;

    case WAIT_FAILED:
        errcode = GetLastError();
        return infra_translate_sys_error(errcode);

    default:
        break;
    }

    abort();
}

#else

#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct infra_exec_data
{
    infra_os_pipe_t     child_pipe[2];
} infra_exec_data_t;

#define INFRA_EXEC_DATA_INIT    \
    {\
        { INFRA_OS_PIPE_INVALID, INFRA_OS_PIPE_INVALID },\
    }

#define NSEC_PER_SEC 1000000000

extern char** environ;

static void uv__write_int(int fd, int val)
{
    ssize_t n;

    do
    {
        n = write(fd, &val, sizeof(val));
    } while (n == -1 && errno == EINTR);

    /*
     * The write might have failed (e.g. if the parent process has died),
     * but we have nothing left.
     */
}

static int _infra_exec_child(const char* file, infra_exec_opt_t* opt, infra_exec_data_t* data)
{
    int errcode = 0;

    /* Close unused peer. */
    infra_pipe_close(data->child_pipe[0]);
    data->child_pipe[0] = INFRA_OS_PIPE_INVALID;

    if (opt != NULL && opt->cwd != NULL)
    {
        if (chdir(opt->cwd) != 0)
        {
            errcode = errno;
            goto finish;
        }
    }

    if (opt != NULL && opt->envs != NULL)
    {
        environ = opt->envs;
    }

    if (opt->h_stdin >= 0)
    {
        dup2(opt->h_stdin, STDIN_FILENO);
    }
    if (opt->h_stdout >= 0)
    {
        dup2(opt->h_stdout, STDOUT_FILENO);
    }
    if (opt->h_stderr >= 0)
    {
        dup2(opt->h_stderr, STDERR_FILENO);
    }

    execvp(file, opt->args);
    errcode = errno;

finish:
    uv__write_int(data->child_pipe[1], errcode);
    return errcode;
}

int infra_exec(infra_pid_t** pid, const char* file, infra_exec_opt_t* opt)
{
    int ret;
    int status;
    int exec_errorno;

    infra_os_pid_t tmp_pid;

    infra_exec_data_t data = INFRA_EXEC_DATA_INIT;
    if ((ret = infra_pipe_open(data.child_pipe, INFRA_OS_PIPE_CLOEXEC, INFRA_OS_PIPE_CLOEXEC)) != 0)
    {
        goto finish;
    }

    if ((tmp_pid = fork()) == 0)
    {
        _infra_exec_child(file, opt, &data);
        abort();
    }
    else if (tmp_pid == -1)
    {
        ret = infra_translate_sys_error(errno);
        goto finish;
    }

    /* Close unused peer. */
    infra_pipe_close(data.child_pipe[1]);
    data.child_pipe[1] = INFRA_OS_PIPE_INVALID;

    /* Wait for child execve result. */
    ret = infra_pipe_read(data.child_pipe[0], &exec_errorno, sizeof(exec_errorno));

    if (ret == INFRA_EOF)
    {
        /* okay, EOF */
        ret = 0;
    }
    else if (ret == sizeof(exec_errorno))
    {
        do 
        {
            ret = waitpid(*pid, &status, 0);
        } while (ret == -1 && errno == EINTR);
        assert(ret == *pid);
        ret = exec_errorno;
    }
    else
    {
        abort();
    }

finish:
    if (data.child_pipe[0] != INFRA_OS_PIPE_INVALID)
    {
        infra_pipe_close(data.child_pipe[0]);
    }
    if (data.child_pipe[1] != INFRA_OS_PIPE_INVALID)
    {
        infra_pipe_close(data.child_pipe[1]);
    }
    return ret;
}

static int _infra_waitpid_inf(infra_os_pid_t pid, int* code)
{
    int wstatus = 0;
    pid_t ret = waitpid(pid, &wstatus, 0);
    if (ret < 0)
    {
        return infra_translate_sys_error(errno);
    }
    assert(ret == pid);

    if (!WIFEXITED(wstatus))
    {
        *code = -1;
    }
    *code = WEXITSTATUS(wstatus);

    return 0;
}

static struct timespec timespec_normalise(struct timespec ts)
{
    while (ts.tv_nsec >= NSEC_PER_SEC)
    {
        ++(ts.tv_sec);
        ts.tv_nsec -= NSEC_PER_SEC;
    }

    while (ts.tv_nsec <= -NSEC_PER_SEC)
    {
        --(ts.tv_sec);
        ts.tv_nsec += NSEC_PER_SEC;
    }

    if (ts.tv_nsec < 0)
    {
        /* Negative nanoseconds isn't valid according to POSIX.
         * Decrement tv_sec and roll tv_nsec over.
        */

        --(ts.tv_sec);
        ts.tv_nsec = (NSEC_PER_SEC + ts.tv_nsec);
    }

    return ts;
}

static struct timespec timespec_add(struct timespec ts1, struct timespec ts2)
{
    /*
     * Normalise inputs to prevent tv_nsec rollover if whole-second values
     * are packed in it.
     */
    ts1 = timespec_normalise(ts1);
    ts2 = timespec_normalise(ts2);

    ts1.tv_sec += ts2.tv_sec;
    ts1.tv_nsec += ts2.tv_nsec;

    return timespec_normalise(ts1);
}

static int timespec_gt(struct timespec ts1, struct timespec ts2)
{
    ts1 = timespec_normalise(ts1);
    ts2 = timespec_normalise(ts2);

    return (ts1.tv_sec > ts2.tv_sec || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec > ts2.tv_nsec));
}

static int _infra_waitpid_timed(infra_os_pid_t pid, int* code, uint32_t ms)
{
    int wstatus = 0;
    pid_t ret = 0;

    struct timespec timeout = { 0, 0 };
    while (ms >= 1000)
    {
        timeout.tv_sec++;
        ms -= 1000;
    }
    timeout.tv_nsec = ms * 1000 * 1000;

    struct timespec t_cur;
    if (clock_gettime(CLOCK_MONOTONIC, &t_cur) < 0)
    {
        return infra_translate_sys_error(errno);
    }

    struct timespec t_dst = timespec_add(t_cur, timeout);

    while (1)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &t_cur) < 0)
        {
            return infra_translate_sys_error(errno);
        }

        ret = waitpid(pid, &wstatus, WNOHANG);
        if (ret > 0)
        {
            assert(ret == pid);
            break;
        }
        else if (ret < 0)
        {
            return infra_translate_sys_error(errno);
        }

        /* Check timeout */
        if (timespec_gt(t_cur, t_dst))
        {
            return INFRA_ETIMEDOUT;
        }

        usleep(10 * 1000);
    }

    if (!WIFEXITED(wstatus))
    {
        *code = -1;
    }
    *code = WEXITSTATUS(wstatus);
    return 0;
}

int infra_waitpid(infra_os_pid_t pid, int* code, uint32_t ms)
{
    int tmp_code = 0;
    if (code == NULL)
    {
        code = &tmp_code;
    }

    if (ms == INFRA_OS_WAITPID_INFINITE)
    {
        return _infra_waitpid_inf(pid, code);
    }

    return _infra_waitpid_timed(pid, code, ms);
}

typedef struct infra_exec_ctx
{
    struct sigaction    old_act;
    infra_mutex_t       guard;
    ev_map_t            child_process_table;
} infra_exec_ctx_t;

static infra_exec_ctx_t* g_exec_ctx = NULL;

static void _infra_exec_handle_sigchld_nolock(infra_pid_t* pid)
{
    int wstatus = 0;
    if (waitpid(pid->sys_pid, &wstatus, WNOHANG) != pid->sys_pid)
    {
        abort();
    }

    if (WIFEXITED(wstatus))
    {
        pid->exit_status = INFRA_PROCESS_EXIT_NORMAL;
        pid->u.exit_code = WEXITSTATUS(wstatus);
    }
    else if (WIFSIGNALED(wstatus))
    {
        pid->exit_status = INFRA_PROCESS_EXIT_SIGNAL;
        pid->u.exit_signal = WTERMSIG(wstatus);
    }
}

static void _infra_exec_on_sigchld(int sig, siginfo_t* info, void* ucontext)
{
    /* Only process SIGCHLD */
    if (sig != SIGCHLD)
    {
        goto process_original;
    }

    /* Find pid */
    infra_pid_t tmp_pid;
    tmp_pid.sys_pid = info->si_pid;

    infra_mutex_enter(&g_exec_ctx->guard);
    {
		ev_map_node_t* it = ev_map_find(&g_exec_ctx->child_process_table, &tmp_pid.node);
		if (it != NULL)
		{
			infra_pid_t* pid = container_of(it, infra_pid_t, node);
            _infra_exec_handle_sigchld_nolock(pid);
		}
    }
    infra_mutex_leave(&g_exec_ctx->guard);

    return;

process_original:
    /* Call original signal handle. */
    if (g_exec_ctx->old_act.sa_flags & SA_SIGINFO)
    {
        if (g_exec_ctx->old_act.sa_sigaction != NULL)
        {
            g_exec_ctx->old_act.sa_sigaction(sig, info, ucontext);
        }
    }
    else if (g_exec_ctx->old_act.sa_handler != NULL)
    {
        g_exec_ctx->old_act.sa_handler(sig);
    }
}

static int _infra_exec_on_cmp_record(const ev_map_node_t* key1, const ev_map_node_t* key2, void* arg)
{
    const infra_pid_t* p1 = container_of(key1, infra_pid_t, node);
    const infra_pid_t* p2 = container_of(key2, infra_pid_t, node);
    if (p1->sys_pid == p2->sys_pid)
    {
        return 0;
    }
    return p1->sys_pid < p2->sys_pid ? -1 : 1;
}

void infra_exec_setup(void)
{
    if ((g_exec_ctx = calloc(1, sizeof(infra_exec_ctx_t))) == NULL)
    {
        abort();
    }
    infra_mutex_init(&g_exec_ctx->guard);
    ev_map_init(&g_exec_ctx->child_process_table, _infra_exec_on_cmp_record, NULL);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    if (sigfillset(&act.sa_mask))
    {
        abort();
    }

    act.sa_flags |= SA_SIGINFO;
    act.sa_sigaction = _infra_exec_on_sigchld;

    if (sigaction(SIGCHLD, &act, &g_exec_ctx->old) != 0)
    {
        abort();
    }
}

#endif
