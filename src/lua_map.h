#ifndef __INFRA_LUA_MAP_INTERNAL_H__
#define __INFRA_LUA_MAP_INTERNAL_H__

#include "lua_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Implementation for infra.new_map()
 * @param[in] L     Lua VM.
 * @return          1 if success, 0 if failure.
 */
API_LOCAL int infra_new_map(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
