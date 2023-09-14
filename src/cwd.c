#include "__init__.h"

#if defined(_WIN32)

#include <assert.h>

static int _infra_cwd(lua_State* L)
{
    DWORD r = GetCurrentDirectoryW(0, NULL);
    WCHAR* w_path = malloc(sizeof(WCHAR) * r);
    if (w_path == NULL)
    {
        goto error_oom;
    }

    DWORD n = GetCurrentDirectoryW(r, w_path);
    assert(n == r - 1);

    const char* path = infra_wide_to_utf8(w_path);
    if (path == NULL)
    {
        free(w_path);
        goto error_oom;
    }

    lua_pushstring(L, path);
    free(w_path);
    return 1;

error_oom:
    return luaL_error(L, "out of memory.");
}

#else

#include <unistd.h>

static int _infra_cwd(lua_State* L)
{
    char* ret = NULL;
    size_t path_sz = 8192;
    char* path = malloc(sizeof(char) * path_sz);
    if (path == NULL)
    {
        goto error_oom;
    }

    while ((ret = getcwd(path, path_sz)) == NULL && (errno == ERANGE))
    {
        size_t new_path_sz = path_sz * 2;
        if ((ret = realloc(path, new_path_sz)) == NULL)
        {
            goto error_oom;
        }
        path = ret;
        path_sz = new_path_sz;
    }
    if (ret == NULL)
    {
        int errcode = infra_translate_sys_error(errno);
        infra_push_error(L, errcode);
        return lua_error(L);
    }

    lua_pushstring(L, path);
    free(path); path = NULL;
    return 1;

error_oom:
    return luaL_error(L, "out of memory.");
}

#endif

const infra_lua_api_t infra_f_cwd = {
"cwd", _infra_cwd, 0,
"Get current working directory.",

"[SYNOPSIS]\n"
"string cwd()\n"
"\n"
"[DESCRIPTION]\n"
"Get current working directory.\n"
};
