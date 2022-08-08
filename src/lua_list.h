#ifndef __INFRA_API_LIST_H__
#define __INFRA_API_LIST_H__

#include "lua_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_NEW_LIST
 */
API_LOCAL int infra_new_list(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
