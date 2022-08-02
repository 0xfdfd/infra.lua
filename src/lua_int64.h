#ifndef __INFRA_API_INT64_H__
#define __INFRA_API_INT64_H__

#include "lua_object.h"
#include <stdint.h>

#define INFRA_LUA_API_UINT64(XX)                                                        \
XX(                                                                                     \
"uint64", infra_uint64, NULL,                                                           \
"Create a native uint64_t number.",                                                     \
"SYNOPSIS"                                                                          "\n"\
"    object uint64(hval, lval);"                                                    "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `uint64()` function create a true unsigned 64-bit object with full"        "\n"\
"    math operators support."                                                       "\n"\
                                                                                    "\n"\
"    The `hval` is higher 32-bit value, and `lval` is lower 32-bit value."          "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A unsigned 64-bit number."                                                     "\n"\
)

#define INFRA_LUA_API_INT64(XX)                                                         \
XX(                                                                                     \
"int64", infra_int64, NULL,                                                             \
"Create a native int64_t number.",                                                      \
"SYNOPSIS"                                                                          "\n"\
"    object int64(hval, lval);"                                                     "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `int64()` function create a true signed 64-bit object with full math"      "\n"\
"    operations support."                                                           "\n"\
                                                                                    "\n"\
"    The `hval` is higher 32-bit value, and `lval` is lower 32-bit value."          "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A signed 64-bit number."                                                       "\n"\
)

typedef struct infra_int64
{
    infra_object_t  object;
    int64_t         value;
} infra_int64_t;

typedef struct infra_uint64
{
    infra_object_t  object;
    uint64_t        value;
} infra_uint64_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_UINT64
 */
API_LOCAL int infra_uint64(lua_State* L);

/**
 * @brief Push a unsigned 64-bit number onto the stack
 * @see #infra_uint64_t
 * @param[out] L    The lua stack
 * @param[in] val   unsigned 64-bit value.
 * @return          Always 1.
 */
API_LOCAL int infra_push_uint64(lua_State* L, uint64_t val);

/**
 * @see INFRA_LUA_API_INT64
 */
API_LOCAL int infra_int64(lua_State* L);

/**
 * @brief Push a signed 64-bit number onto the stack
 * @see #infra_int64_t
 * @param[out] L    The lua stack
 * @param[in] val   signed 64-bit value.
 * @return          Always 1.
 */
API_LOCAL int infra_push_int64(lua_State* L, int64_t val);

#ifdef __cplusplus
}
#endif

#endif
