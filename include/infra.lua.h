#ifndef __INFRA_LUA_H__
#define __INFRA_LUA_H__

#include <infra.lua/map.h>

#ifdef __cplusplus
extern "C" {
#endif

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
