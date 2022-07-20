#include <infra.lua.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static const char* s_help =
"infra_test [PATH_TO_FILE]\n"
;

static int pmain (lua_State *L)
{
    int argc = (int)lua_tointeger(L, 1);
    char** argv = lua_touserdata(L, 2);

    if (argc < 2)
    {
        fprintf(stdout, "%s", s_help);
        lua_pushinteger(L, EXIT_FAILURE);
        return 1;
    }

    luaL_openlibs(L);
    luaL_requiref(L, "infra", luaopen_infra, 1);
    lua_pop(L, 1);

    if (luaL_dofile(L, argv[1]) != LUA_OK)
    {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pushinteger(L, EXIT_FAILURE);
        return 1;
    }

    lua_pushinteger(L, EXIT_SUCCESS);
    return 1;
}

int main(int argc, char* argv[])
{
    lua_State *L = luaL_newstate();
    assert(L != NULL);

    lua_pushcfunction(L, pmain);
    lua_pushinteger(L, argc);
    lua_pushlightuserdata(L, argv);

    int status = lua_pcall(L, 2, 1, 0);
    int result = (int)lua_tointeger(L, -1);

    lua_close(L);
    return status == LUA_OK ? result : EXIT_FAILURE;
}
