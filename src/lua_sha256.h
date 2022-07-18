#ifndef __INFRA_API_SHA256_H__
#define __INFRA_API_SHA256_H__

#include "lua_api.h"

#define INFRA_LUA_API_SHA256_ONCE(XX)                                                   \
XX(                                                                                     \
"sha256_once", infra_sha256_once, NULL,                                                 \
"Calculate SHA256 of given data.",                                                      \
"SYNOPSIS"                                                                          "\n"\
"    string sha256_once(string);"                                                   "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `sha256_once()` function read content of `string` and calculate SHA256"    "\n"\
"    for it."                                                                       "\n"\
                                                                                    "\n"\
"    The result is a 64 bytes string holding SHA256 result in uppercase."           "\n"\
                                                                                    "\n"\
"EXAMPLES"                                                                          "\n"\
"    The code below calculate SHA256 of string `hello infra` and print to stdout."  "\n"\
                                                                                    "\n"\
"    ```lua"                                                                        "\n"\
"    local infra = require(\"infra\")"                                              "\n"\
"    local sha256 = infra.sha256_once(\"hello infra\")"                             "\n"\
"    io.write(sha256)"                                                              "\n"\
"    ```"                                                                           "\n"\
                                                                                    "\n"\
"    And the output should be:"                                                     "\n"\
"    C551B909B1E21EADFFE8F23B5D8A1AA8BB7F16D45F7A8A5B1A917E9CD4AB5C63"              "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

API_LOCAL int infra_sha256_once(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
