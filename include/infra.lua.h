#ifndef __INFRA_LUA_H__
#define __INFRA_LUA_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#   define INFRA_API    __declspec(dllexport)
#else
#   define INFRA_API
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/**
 * @brief Entrypoint for infra.
 * @param[in] L Lua VM.
 * @return      Always 1.
 */
INFRA_API int luaopen_infra(lua_State* L);

#ifdef __cplusplus
}
#endif
#endif
