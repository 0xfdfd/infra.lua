#ifndef __INFRA_API_JSON_H__
#define __INFRA_API_JSON_H__

#include "lua_api.h"

#define INFRA_LUA_API_TABLE_TO_JSON(XX)                                                 \
XX(                                                                                     \
"table_to_json", infra_table_to_json, NULL,                                             \
"Convert table to json string.",                                                        \
"SYNOPSIS"                                                                          "\n"\
"    string table_to_json(table);"                                                  "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `table_to_json()` function convert table to json string."                  "\n"\
                                                                                    "\n"\
"    Due to `nil` in lua means \"delete item in table\", it is not possible use"    "\n"\
"    native type system to stand for `null` in json. To place `null`, use infra"    "\n"\
"    `null()` function."                                                            "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A string that can be serialize to json object."                                "\n"\
                                                                                    "\n"\
"EXAMPLES"                                                                          "\n"\
"    The following code convert a lua object to json string:"                       "\n"\
                                                                                    "\n"\
"    ```lua"                                                                        "\n"\
"    local infra = require(\"infra\")"                                              "\n"\
"    local test_table = { a = infra.null() }"                                       "\n"\
"    io.write(infra.table_to_json())"                                               "\n"\
"    ```"                                                                           "\n"\
)

#define INFRA_LUA_API_JSON_TO_TABLE(XX)                                                 \
XX(                                                                                     \
"json_to_table", infra_json_to_table, NULL,                                             \
"Convert json string to table.",                                                        \
"SYNOPSIS"                                                                          "\n"\
"    table json_to_table(string);"                                                  "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `json_to_table()` function convert json string to lua table."              "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A table that equal to original json string."                                   "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_TABLE_TO_JSON
 */
API_LOCAL int infra_table_to_json(lua_State* L);

/**
 * @see INFRA_LUA_API_JSON_TO_TABLE
 */
API_LOCAL int infra_json_to_table(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
