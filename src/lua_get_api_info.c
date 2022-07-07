#include "lua_get_api_info.h"

static int _on_each_api(const infra_lua_api_t* api, void* arg)
{
    lua_State* L = arg;

    lua_pushstring(L, api->name);
    lua_newtable(L);

    lua_pushstring(L, "brief");
    lua_pushstring(L, api->brief);
    lua_settable(L, -3);

    lua_pushstring(L, "document");
    lua_pushstring(L, api->document);
    lua_settable(L, -3);

    lua_settable(L, -3);

    return 0;
}

int infra_get_api_info(lua_State* L)
{
    lua_newtable(L);
    lua_api_foreach(_on_each_api, L);
    return 1;
}
