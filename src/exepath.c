#include "__init__.h"

#if !defined(_WIN32)
#   include <unistd.h>
#endif

static int _infra_exepath(lua_State* L)
{
    size_t path_sz = 4096;

#if defined(_WIN32)

    WCHAR* path = malloc(sizeof(WCHAR) * path_sz);
    if (path == NULL)
    {
        return luaL_error(L, "out of memory.");
    }

    GetModuleFileNameW(NULL, path, (DWORD)path_sz);
    infra_lua_wide_to_utf8(L, path);
    free(path);

#else

    char* path = malloc(sizeof(char) * path_sz);
    if (path == NULL)
    {
        return luaL_error(L, "out of memory.");
    }

    ssize_t ret = readlink("/proc/self/exe", path, path_sz);
    if (ret < 0)
    {
        int errcode = errno;
        errcode = infra_translate_sys_error(errcode);
        infra_push_error(L, errcode);
        return lua_error(L);
    }

    lua_pushlstring(L, path, ret);

#endif
    return 1;
}

const infra_lua_api_t infra_f_exepath = {
"exepath", _infra_exepath, 0,
"Gets the executable path.",

"[SYNOPSIS]\n"
"string exepath()\n"
};
