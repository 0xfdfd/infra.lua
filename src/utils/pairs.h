#ifndef __INFRA_LUA_UTILS_PAIRS_H__
#define __INFRA_LUA_UTILS_PAIRS_H__

#include "lua_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Iterator callback.
 * @param[in] L     Lua VM.
 * @param[in] sp    The current SP. sp == lua_gettop(L).
 * @param[in] kidx  Key index.
 * @param[in] vidx  Value index.
 * @param[in] arg   User defined argument.
 * @return          0 to stop iterator, otherwise continue.
 */
typedef int (*infra_pairs_foreach_cb)(lua_State* L, int sp, int kidx, int vidx,
    void* arg);

/**
 * @brief Iterator all elements in object that has `__pairs` meta method.
 * @param[in] L     Lua VM.
 * @param[in] idx   Object index that have `__pairs` meta method.
 * @param[in] cb    Iterator callback, return 0 to stop iterator.
 * @param[in] arg   Pass to \p cb.
 * @return          1 if success, 0 if object at \p idx does not have `__pairs`
 *                  meta method.
 */
API_LOCAL int infra_pairs_foreach(lua_State* L, int idx,
    infra_pairs_foreach_cb cb, void* arg);

#ifdef __cplusplus
}
#endif
#endif
