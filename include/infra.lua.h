#ifndef __INFRA_LUA_H__
#define __INFRA_LUA_H__

#include <infra.lua/list.h>
#include <infra.lua/map.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup INFRA infra
 *
 * A library to enhance lua capability.
 *
 * @{
 */

/**
 * @brief Entrypoint for infra.
 *
 * Use `require('infra')` to load infra library.
 *
 * @param[in] L Lua VM.
 * @return      Always 1.
 */
INFRA_API int luaopen_infra(lua_State* L);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
