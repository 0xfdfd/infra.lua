#include "__init__.h"

static int _infra_pairs_next(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);
    if (lua_next(L, 1))
    {
        return 2;
    }

    lua_pushnil(L);
    return 1;
}

#if LUA_VERSION_NUM >= 503
static int _infra_pairscont(lua_State* L, int status, lua_KContext k)
{
    (void)L; (void)status; (void)k;  /* unused */
    return 3;
}
#endif

static int _infra_pairs(lua_State* L)
{
    if (luaL_getmetafield(L, 1, "__pairs") == LUA_TNIL)
    {
        lua_pushcfunction(L, _infra_pairs_next);
        lua_pushvalue(L, 1);
        lua_pushnil(L);
        return 3;
    }

    lua_pushvalue(L, 1);

#if LUA_VERSION_NUM >= 503
    lua_callk(L, 1, 3, 0, _infra_pairscont);
#else
    lua_call(L, 1, 3);
#endif
    return 3;
}

const infra_lua_api_t infra_f_pairs = {
"pairs", _infra_pairs, 0,
"Compatible `pairs` function for lua5.1 and luajit.",

"[SYNOPSIS]\n"
"next,t,nil pairs(t)\n"
"\n"
"[DESCRIPTION]\n"
"If `t` has a meta-method `__pairs`, calls it with t as argument and returns the\n"
"first three results from the call.\n"
"\n"
"Otherwise, returns three values: the `next` function, the table `t`, and nil,\n"
"so that the construction:\n"
"```lua\n"
"for k,v in pairs(t) do body end\n"
"```"
"will iterate over all key¨Cvalue pairs of table `t`.\n"
};
