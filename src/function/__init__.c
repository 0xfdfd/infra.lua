#include "__init__.h"
#include <stdlib.h>
#include <assert.h>

/**
 * @brief Error code map for GetLastError()
 * @see https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
 * @see https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
 */
#define INFRA_SYS_ERRNO_WIN32_MAP(xx)   \
    xx(WAIT_TIMEOUT,        INFRA_ETIMEDOUT)   \
    xx(ERROR_BROKEN_PIPE,   INFRA_EOF)

/**
 * @brief Error code map for errno.
 * @see https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
 */
#define INFRA_SYS_ERRNO_POSIX_MAP(xx)   \
    xx(ETIMEDOUT,           INFRA_ETIMEDOUT)

typedef struct infra_errno_map
{
    int src;
    int dst;
} infra_errno_map_t;

/**
 * @brief Infra API.
 */
static const infra_lua_api_t* s_api[] = {
    &infra_f_argparser,
    &infra_f_basename,
    &infra_f_compare,
    &infra_f_cwd,
    &infra_f_dirname,
    &infra_f_dump_any,
    &infra_f_dump_hex,
    &infra_f_execute,
    &infra_f_exepath,
    &infra_f_man,
    &infra_f_map,
    &infra_f_merge_line,
    &infra_f_pairs,
    &infra_f_range,
    &infra_f_readdir,
    &infra_f_readfile,
    &infra_f_split_line,
    &infra_f_strcasecmp,
    &infra_f_writefile,
};

void lua_api_foreach(lua_api_foreach_fn cb, void* arg)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_api); i++)
    {
        if (cb(s_api[i], arg) != 0)
        {
            break;
        }
    }
}

int infra_typeerror(lua_State *L, int arg, const char *tname)
{
#if LUA_VERSION_NUM >= 504
    return luaL_typeerror(L, arg, tname);
#else
    lua_Debug ar;
    if (!lua_getstack(L, 0, &ar))
    {
        return luaL_error(L, "bad argument #%d (%s expected, got %s)",
            arg, tname, luaL_typename(L, arg));
    }
    lua_getinfo(L, "n", &ar);
    return luaL_error(L, "bad argument #%d to '%s' (%s expected, got %s)",\
                arg, ar.name ? ar.name : "?", tname, luaL_typename(L, arg));
#endif
}

void* infra_tmpbuf(lua_State* L, size_t size)
{
    const size_t alignment = sizeof(void*) * 2;
    void* addr = lua_newuserdata(L, size + alignment);
    return (void*)ALIGN_SIZE(addr, alignment);
}

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

#if defined(_WIN32)

char* infra_wide_to_utf8(const WCHAR* str)
{
    int utf8_sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char* utf8 = malloc(utf8_sz + 1);
    if (utf8 == NULL)
    {
        return NULL;
    }

    int ret = WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8, utf8_sz, NULL, NULL);
    assert(ret == utf8_sz);
    utf8[utf8_sz] = '\0';

    return utf8;
}

char* infra_lua_wide_to_utf8(lua_State* L, const WCHAR* str)
{
    int utf8_sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char* utf8 = malloc(utf8_sz + 1);
    if (utf8 == NULL)
    {
        return NULL;
    }

    int ret = WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8, utf8_sz, NULL, NULL);
    assert(ret == utf8_sz);
    utf8[utf8_sz] = '\0';

    lua_pushlstring(L, utf8, utf8_sz);
    free(utf8); utf8 = NULL;

    return (char*)lua_tostring(L, -1);
}

WCHAR* infra_utf8_to_wide(const char* str)
{
    int multi_byte_sz = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    size_t buf_sz = multi_byte_sz * sizeof(WCHAR);

    WCHAR* buf = (WCHAR*)malloc(buf_sz);
    int ret = MultiByteToWideChar(CP_UTF8, 0, str, -1, (WCHAR*)buf, multi_byte_sz);
    assert(ret == multi_byte_sz);

    return buf;
}

WCHAR* infra_lua_utf8_to_wide(lua_State* L, const char* str)
{
    int multi_byte_sz = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    size_t buf_sz = multi_byte_sz * sizeof(WCHAR);

    WCHAR* buf = (WCHAR*)malloc(buf_sz);
    int ret = MultiByteToWideChar(CP_UTF8, 0, str, -1, (WCHAR*)buf, multi_byte_sz);
    assert(ret == multi_byte_sz);

    lua_pushlstring(L, (const char*)buf, buf_sz);
    free(buf); buf = NULL;

    return (WCHAR*)lua_tostring(L, -1);
}

void infra_push_error(lua_State* L, int errcode)
{
    assert(errcode <= 0);
    errcode = -errcode;

    char buf[256];
    strerror_s(buf, sizeof(buf), errcode);

    lua_pushstring(L, buf);
}

#else

int fopen_s(FILE** pFile, const char* filename, const char* mode)
{
    if ((*pFile = fopen(filename, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}

void infra_push_error(lua_State* L, int errcode)
{
    assert(errcode <= 0);
    errcode = -errcode;

    lua_pushstring(L, strerror(errcode));
}

#endif

#if defined(INFRA_NEED_LUA_ABSINDEX)
int infra_lua_absindex(lua_State* L, int idx)
{
    if (idx < 0 && idx > LUA_REGISTRYINDEX)
    {
        idx = lua_gettop(L) + idx + 1;
    }
    return idx;
}
#endif

#if defined(INFRA_NEED_LUAL_LEN)
lua_Integer infra_luaL_len(lua_State* L, int idx)
{
    return lua_objlen(L, idx);
}
#endif

#if defined(INFRA_NEED_LUA_GETFIELD)
int infra_lua_getfield(lua_State* L, int idx, const char* k)
{
    /*
     * Let's call the original version of `lua_getfield()`.
     */
    (lua_getfield)(L, idx, k);
    return lua_type(L, -1);
}
#endif

#if defined(INFRA_NEED_LUA_GETTABLE)
int infra_lua_gettable(lua_State* L, int idx)
{
    (lua_gettable)(L, idx);
    return lua_type(L, -1);
}
#endif

#if defined(INFRA_NEED_LUA_GETI)
int infra_lua_geti(lua_State* L, int idx, lua_Integer n)
{
    idx = lua_absindex(L, idx);

    lua_pushinteger(L, n);
    lua_gettable(L, idx);

    return lua_type(L, -1);
}
#endif

#if defined(INFRA_NEED_LUA_SETI)
void infra_lua_seti(lua_State* L, int idx, lua_Integer n)
{
    idx = lua_absindex(L, idx);
    int sp = lua_gettop(L);

    lua_pushinteger(L, n);
    lua_insert(L, sp);

    lua_settable(L, idx);
}
#endif

#if defined(INFRA_NEED_LUAL_SETFUNCS)
void infra_luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup)
{
    for (; l->name != NULL; l++)
    {
        if (l->func == NULL)
        {
            lua_pushboolean(L, 0);
        }
        else
        {
            int i;
            for (i = 0; i < nup; i++)
            {
                lua_pushvalue(L, -nup);
            }
            lua_pushcclosure(L, l->func, nup);
        }
        lua_setfield(L, -(nup + 2), l->name);
    }

    lua_pop(L, nup);
}
#endif

#if defined(INFRA_NEED_LUA_TONUMBERX)
lua_Number infra_lua_tonumberx(lua_State* L, int idx, int* isnum)
{
    int tmp_isnum;
    if (isnum == NULL)
    {
        isnum = &tmp_isnum;
    }

    int ret;
    if ((ret = lua_type(L, idx)) == LUA_TNUMBER)
    {
        *isnum = 1;
        return lua_tonumber(L, idx);
    }
    else if (ret == LUA_TSTRING)
    {
        const char* s = lua_tostring(L, idx);
        double v = 0;
        if (sscanf(s, "%lf", &v) == 1)
        {
            *isnum = 1;
            return v;
        }
    }

    *isnum = 0;
    return 0;
}
#endif

#if defined(INFRA_NEED_LUAL_NEWLIB)
void infra_luaL_newlib(lua_State* L, const luaL_Reg l[])
{
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
}
#endif
