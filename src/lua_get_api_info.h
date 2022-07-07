#ifndef __INFRA_API_GET_API_INFO_H__
#define __INFRA_API_GET_API_INFO_H__

#include "lua_api.h"

#define INFRA_LUA_API_GET_API_INFO(XX)                                                  \
XX(                                                                                     \
"get_api_info", infra_get_api_info, NULL,                                               \
"Get a table contains all infra API info.",                                             \
"SYNOPSIS"                                                                          "\n"\
"    table get_api_info();"                                                         "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `get_api_info()` function collect all infra API and return as a table."    "\n"\
                                                                                    "\n"\
"    The table have following structure:"                                           "\n"\
"    ```"                                                                           "\n"\
"    {"                                                                             "\n"\
"        [name] = {"                                                                "\n"\
"            [brief] = \"Brief document\","                                         "\n"\
"            [document] = \"Detail document\""                                      "\n"\
"        }"                                                                         "\n"\
"    }"                                                                             "\n"\
"    ```"                                                                           "\n"\
                                                                                    "\n"\
"    The `name` is name of function, `brief` is short description of this"          "\n"\
"    function, `document` is detail description of this function."                  "\n"\
                                                                                    "\n"\
"EXAMPLES"                                                                          "\n"\
"    Here is an example of print all infra API name and brief document:"            "\n"\
                                                                                    "\n"\
"    ```lua"                                                                        "\n"\
"    local infra = require(\"infra\")"                                              "\n"\
"    local records = infra.get_api_info()"                                          "\n"\
"    for name, value in ipairs(records) do"                                         "\n"\
"        io.write(name .. \" - \" .. value.brief .. \"\\n\")"                       "\n"\
"    end"                                                                           "\n"\
"    ```"                                                                           "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

API_LOCAL int infra_get_api_info(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
