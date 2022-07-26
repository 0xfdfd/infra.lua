#include "lua_constant.h"

int infra_push_null(lua_State* L)
{
    lua_pushlightuserdata(L, NULL);
    return 1;
}
