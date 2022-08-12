#include "pairs.h"

int infra_pairs_foreach(lua_State* L, int idx, infra_pairs_foreach_cb cb,
    void* arg)
{
    int sp = lua_gettop(L);

    int val_type = luaL_getmetafield(L, idx, "__pairs");
    if (val_type == LUA_TNIL)
    {
        return 0;
    }

    lua_pushvalue(L, idx);
    lua_call(L, 1, 3);

    /*
     * SP+1: next function
     * SP+2: table
     * SP+3: nil
     */

    int next_func = sp + 1;
    int user_obj = sp + 2;

    lua_pushvalue(L, next_func);
    lua_pushvalue(L, user_obj);
    lua_pushnil(L);

    for (;;)
    {
        /*
         * Required stack layout:
         * SP+1: next function
         * SP+2: table
         * SP+3: nil
         * SP+4: next function
         * SP+5: table
         * SP+6: key
         */
        lua_call(L, 2, 2);

        /*
         * After call:
         * SP+1: next function
         * SP+2: table
         * SP+3: nil
         * SP+4: key
         * SP+5: value
         */

        if (lua_type(L, sp + 4) == LUA_TNIL)
        {
            break;
        }

        if (cb(L, sp + 4, sp + 5, arg) == 0)
        {
            break;
        }

        /* Avoid \p cb disturb stack */
        lua_settop(L, sp + 5);

        /* Push key to SP+6 */
        lua_pushvalue(L, sp + 4);
        /* Replace SP+4 with next function */
        lua_pushvalue(L, next_func);
        lua_replace(L, sp + 4);
        /* Replace SP+5 with table object */
        lua_pushvalue(L, user_obj);
        lua_replace(L, sp + 5);
    }

    /* Restore stack */
    lua_settop(L, sp);

    return 1;
}
