#include "lua_api.h"
#include "lua_errno.h"
#include "lua_get_api_info.h"
#include "lua_sha256.h"
#include "lua_task.h"

/**
 * @brief Infra API.
 * 
 * Infra API is describe by a set of macros, each macro must satisfy following
 * field:
 * + name: A C string contains the name of API.
 * + func: A function pointer for real C function.
 * + init: A function pointer for initialize module.
 * + brief: A C string contains brief document.
 * + document: A C string contains detail document.
 */
#define INFRA_LUA_APIS(XX)                  \
    INFRA_LUA_API_STRERROR(XX)              \
    INFRA_LUA_API_GET_API_INFO(XX)          \
    INFRA_LUA_API_SHA256_ONCE(XX)           \
    INFRA_LUA_API_TASK_RUN(XX)              \
    INFRA_LUA_API_TASK_SPAWN(XX)            \
    INFRA_LUA_API_TASK_READY(XX)            \
    INFRA_LUA_API_TASK_WAIT(XX)             \
    INFRA_LUA_API_TASK_INFO(XX)

#define EXPAND_INFRA_APIS_AS_REG(name, func, init, brief, document)   \
    { name, func },
static const luaL_Reg s_infra_api_reg[] = {
    INFRA_LUA_APIS(EXPAND_INFRA_APIS_AS_REG)
    { NULL, NULL }
};
#undef EXPAND_INFRA_APIS_AS_REG

#define EXPAND_INFRA_APIS_AS_INFO(name, func, init, brief, document)   \
    { name, func, init, brief, document },
static const infra_lua_api_t s_infra_api_info[] = {
    INFRA_LUA_APIS(EXPAND_INFRA_APIS_AS_INFO)
    { NULL, NULL, NULL, NULL, NULL }
};
#undef EXPAND_INFRA_APIS_AS_INFO

static void _infra_load_init(lua_State* L)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_infra_api_info); i++)
    {
        if (s_infra_api_info[i].init != NULL)
        {
            s_infra_api_info[i].init(L);
        }
    }
}

int luaopen_infra(lua_State* L)
{
    luaL_checkversion(L);
    luaL_newlib(L, s_infra_api_reg);
    _infra_load_init(L);
    return 1;
}

void lua_api_foreach(lua_api_foreach_fn cb, void* arg)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_infra_api_info); i++)
    {
        if (s_infra_api_info[i].func == NULL)
        {
            continue;
        }

        if (cb(&s_infra_api_info[i], arg) != 0)
        {
            break;
        }
    }
}
