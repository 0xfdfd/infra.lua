#include "function/__init__.h"
#include "utils/exec.h"
#include "utils/once.h"

typedef struct infra_lua_open_helper
{
    lua_State*  L;
    int         idx_ret;
} infra_lua_open_helper_t;

static int _infra_lua_foreach_api(const infra_lua_api_t* api, void* arg)
{
    infra_lua_open_helper_t* helper = arg;

    if (api->upvalue)
    {
        lua_pushvalue(helper->L, helper->idx_ret);
        lua_pushcclosure(helper->L, api->addr, 1);
    }
    else
    {
        lua_pushcfunction(helper->L, api->addr);
    }
    lua_setfield(helper->L, helper->idx_ret, api->name);

    return 0;
}

static void _infra_lua_setup_once(void)
{
    /*
     * Register global cleanup hook.
     */
    atexit(infra_lua_cleanup);

    infra_exec_setup();
}

INFRA_API int luaopen_infra(lua_State* L)
{
    int sp = lua_gettop(L);

    {
        static infra_once_t token = INFRA_ONCE_INIT;
        infra_once(&token, _infra_lua_setup_once);
    }

    lua_newtable(L);

    infra_lua_open_helper_t helper;
    memset(&helper, 0, sizeof(helper));
    helper.L = L;
    helper.idx_ret = sp + 1;

    lua_api_foreach(_infra_lua_foreach_api, &helper);

    return 1;
}

INFRA_API void infra_lua_cleanup(void)
{
    infra_exec_cleanup();
}
