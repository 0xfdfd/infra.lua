#ifndef __INFRA_LUA_API_EQUAL_H__
#define __INFRA_LUA_API_EQUAL_H__

#include "lua_api.h"

#define INFRA_LUA_API_EQUAL(XX)                                                         \
XX(                                                                                     \
"equal", infra_equal, NULL,                                                             \
"Compares two Lua values.",                                                             \
"SYNOPSIS"                                                                          "\n"\
"    bool equal(v1, v2);"                                                           "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `equal()` function deep compare two lua values."                           "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    true if equal, false if not."                                                  "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_EQUAL
 */
API_LOCAL int infra_equal(lua_State* L);

/**
 * This function remain stack balance.
 * @param[in] L     Lua VM.
 * @param[in] idx1  Value at index1.
 * @param[in] idx2  Value at index2.
 * @return          boolean
 */
API_LOCAL int infra_is_equal(lua_State* L, int idx1, int idx2);

#ifdef __cplusplus
}
#endif

#endif
