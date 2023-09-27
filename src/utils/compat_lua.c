#include "compat_lua.h"

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
