#include "lua_equal.h"

/**
 *
 * @param L
 * @param idx1
 * @param idx2
 * @return 1 if equal, 0 of not.
 */
static int _equal_deep_compare_table(lua_State* L, int idx1, int idx2)
{
    int ret = 1;
    int sp = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, idx1) != 0)
    {
        /* Get value type */
        lua_type(L, sp + 2);
        /* Find key in idx2 */
        lua_pushvalue(L, sp + 1);
        lua_gettable(L, idx2);

        if ((ret = infra_is_equal(L, sp + 2, sp + 3)) == 0)
        {
            goto finish;
        }

        lua_pop(L, 2);
    }

finish:
    lua_settop(L, sp);
    return ret;
}

int infra_is_equal(lua_State* L, int idx1, int idx2)
{
    int v1_type = lua_type(L, idx1);
    int v2_type = lua_type(L, idx2);
    if (v1_type != v2_type)
    {
        return 0;
    }

    if (v1_type != LUA_TTABLE)
    {
        return lua_compare(L, idx1, idx2, LUA_OPEQ);
    }

    v1_type = luaL_getmetafield(L, 1, "__eq");
    if (v1_type != LUA_TFUNCTION)
    {/* No __eq function */
        return _equal_deep_compare_table(L, idx1, idx2)
            && _equal_deep_compare_table(L, idx2, idx1);
    }

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);

    int ret = lua_toboolean(L, -1);
    lua_pop(L, 1);

    return ret;
}

int infra_equal(lua_State* L)
{
    lua_pushboolean(L, infra_is_equal(L, 1, 2));
    return 1;
}
