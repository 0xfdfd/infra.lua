#include "__init__.h"
#include <stdlib.h>
#include <assert.h>

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

void infra_push_error(lua_State* L, int errcode)
{
    assert(errcode <= 0);
    errcode = -errcode;

    lua_pushstring(L, strerror(errcode));
}

#endif
