#include "__init__.h"
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Infra API.
 */
static const infra_lua_api_t* s_api[] = {
    &infra_f_basename,
    &infra_f_compare,
    &infra_f_dirname,
    &infra_f_dump_any,
    &infra_f_dump_hex,
    &infra_f_man,
    &infra_f_map,
    &infra_f_merge_line,
    &infra_f_readdir,
    &infra_f_split_line,
    &infra_f_strcasecmp,
};

INFRA_API int luaopen_infra(lua_State* L)
{
    size_t i;

#if defined(luaL_checkversion)
    luaL_checkversion(L);
#endif

    int sp = lua_gettop(L);
    lua_newtable(L); // sp+1

    for (i = 0; i < ARRAY_SIZE(s_api); i++)
    {
        if (s_api[i]->upvalue)
        {
            lua_pushvalue(L, sp + 1); // sp+2
            lua_pushcclosure(L, s_api[i]->addr, 1); // sp+2
        }
        else
        {
            lua_pushcfunction(L, s_api[i]->addr); // sp+2
        }

        lua_setfield(L, -2, s_api[i]->name); // sp+1
    }

    return 1;
}

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

#if !defined(_MSC_VER)
int fopen_s(FILE** pFile, const char* filename, const char* mode)
{
    if ((*pFile = fopen(filename, mode)) == NULL)
    {
        return errno;
    }
    return 0;
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

#if defined(INFRA_NEED_LUAL_NEWLIB)
void infra_luaL_newlib(lua_State* L, const luaL_Reg l[])
{
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
}
#endif
