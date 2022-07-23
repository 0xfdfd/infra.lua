#ifndef __INFRA_API_DUMP_H__
#define __INFRA_API_DUMP_H__

#include "lua_api.h"

#define INFRA_LUA_API_DUMP_VALUE_AS_STRING(XX)                                          \
XX(                                                                                     \
"dump_value_as_string", infra_dump_value_as_string, NULL,                               \
"Dump value as c string.",                                                              \
"SYNOPSIS"                                                                          "\n"\
"    string dump_value_as_string(value);"                                           "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `dump_value_as_string()` function convert `value` as c string, without"    "\n"\
"    modify original value type:"                                                   "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A string contains human readable content."                                     "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_DUMP_VALUE_AS_STRING
 */
API_LOCAL int infra_dump_value_as_string(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
