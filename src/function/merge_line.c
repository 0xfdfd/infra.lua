#include "__init__.h"

static int _merge_line_by_table(lua_State* L, const char* delim, size_t delim_sz)
{
    int sp = lua_gettop(L);
    lua_pushstring(L, ""); // sp+1: the return value.

    /* Concat */
    lua_Integer i;
    for (i=1; ; i++)
    {
        int concat_sz = 2;
        if (i != 1)
        {
            lua_pushlstring(L, delim, delim_sz);
            concat_sz = 3;
        }

        if (lua_geti(L, 1, i) != LUA_TSTRING)
        {
            break;
        }

        lua_concat(L, concat_sz);
    }

    lua_settop(L, sp + 1);
    return 1;
}

static int _merge_line_by_pairs(lua_State* L, const char* delim, size_t delim_sz)
{
    int sp = lua_gettop(L);
    lua_pushstring(L, ""); // sp+1: the return value.

    /*
     * sp+2: iterator function
     * sp+3: the object it self
     * sp+4: `nil`
     */
    luaL_callmeta(L, 1, "__pairs");

    size_t i;
    for (i = 1; ; i++)
    {
        /*
         * sp+5: next key.
         * sp+6: next value.
         */
        lua_pushvalue(L, sp + 2);
        lua_pushvalue(L, sp + 3);
        lua_pushvalue(L, sp + 4);
        lua_call(L, 2, 2);
        if (lua_type(L, -1) == LUA_TNIL)
        {
            lua_pop(L, 2);
            break;
        }

        /* Handle sp+6 */
        int concat_sz = 2;
        lua_pushvalue(L, sp + 1);
        if (i != 1)
        {
            lua_pushlstring(L, delim, delim_sz);
            concat_sz = 3;
        }
        lua_pushvalue(L, sp + 6);
        lua_concat(L, concat_sz);
        lua_replace(L, sp + 1);
        lua_pop(L, 1);

        /* Handle sp+5 */
        lua_replace(L, sp + 4);
    }
    lua_pop(L, 3);

    return 1;
}

static int _merge_line(lua_State* L)
{
    const char* delim = luaL_optstring(L, 2, "\n");
    size_t delim_sz = strlen(delim);

    /* Native lua table */
    if (lua_type(L, 1) == LUA_TTABLE)
    {
        return _merge_line_by_table(L, delim, delim_sz);
    }

    /* Custom object that have `__index` metamethod */
    if (luaL_getmetafield(L, 1, "__index") != LUA_TNIL)
    {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            lua_pop(L, 1);
            return _merge_line_by_table(L, delim, delim_sz);
        }
        lua_pop(L, 1);
    }

    /* Custom object that have `__pairs` metamethod */
    if (luaL_getmetafield(L, 1, "__pairs") != LUA_TNIL)
    {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            lua_pop(L, 1);
            return _merge_line_by_pairs(L, delim, delim_sz);
        }
        lua_pop(L, 1);
    }

    return 0;
}

const infra_lua_api_t infra_f_merge_line = {
"merge_line", _merge_line, 0,
"Merge array of strings into a large string.",

"[SYNOPSIS]\n"
"string merge_line(table t [, string token])\n"
"\n"
"[DESCRIPTION]\n"
"Merge array of strings into a large string, with `token` specific combinator.\n"
"If `token` is not assign, it is treat as `\\n`.\n"
"\n"
"EXAMPLE\n"
"If we have following table:\n"
"```lua\n"
"{ [1]=\"hello\", [2]=\" \", [3]=\"world\" }\n"
"```\n"
"The result of merge_line() will be:\n"
"\"hello\\n \\nworld\"."
};
