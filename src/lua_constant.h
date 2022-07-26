#ifndef __INFRA_API_CONSTANT_H__
#define __INFRA_API_CONSTANT_H__

#include "lua_api.h"

#define INFRA_LUA_API_NULL(XX)                                                          \
XX(                                                                                     \
"null", infra_push_null, NULL,                                                          \
"Return NULL for non-existing value.",                                                  \
"SYNOPSIS"                                                                          "\n"\
"    NULL null();"                                                                  "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `null()` function return a constant value `NULL`."                         "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A constant value stand for `NULL`."                                            "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Push a NULL value onto the stack.
 * @see INFRA_LUA_API_NULL
 */
API_LOCAL int infra_push_null(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
