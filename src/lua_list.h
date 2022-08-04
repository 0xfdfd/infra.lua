#ifndef __INFRA_API_LIST_H__
#define __INFRA_API_LIST_H__

#include "lua_api.h"

#define INFRA_LUA_API_NEW_LIST(XX)                                                      \
XX(                                                                                     \
"new_list", infra_new_list, NULL,                                                       \
"Create a new list",                                                                    \
"SYNOPSIS"                                                                          "\n"\
"    list new_list();"                                                              "\n"\
"    list new_list(table);"                                                         "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `new_list()` create a new list and return. If argument is a lua native"    "\n"\
"    list, the content is copied in order."                                         "\n"\
                                                                                    "\n"\
"    The list have following method:"                                               "\n"\
"    + `integer size()`: Get the number of element in list."                        "\n"\
"    + `iter push_front(v)`: Push value into front of list and return iterator."    "\n"\
"    + `iter push_back(v)`: Push value into end of list and return iterator."       "\n"\
"    + `v pop_front()`: Pop front value of list and return it."                     "\n"\
"    + `v pop_back()`: Pop end value of list and return it."                        "\n"\
"    + `void erase(iter)`: Erase value at iterator."                                "\n"\
"    + `iter begin()`: Get iterator point to begin of list. A return value of"      "\n"\
"      `nil` means list is empty."                                                  "\n"\
"    + `iter next(iter)`: Get next iterator. A return value of `nil` means no"      "\n"\
"      more element in list."                                                       "\n"\
                                                                                    "\n"\
"    The list support pairs operation, so you can iterate over all values of"       "\n"\
"    a list by:"                                                                    "\n"\
"    ```lua"                                                                        "\n"\
"    for k,v in pairs(t) do body end"                                               "\n"\
"    ```"                                                                           "\n"\
"    The k in loop is the internal index of value. You are safe to erase this"      "\n"\
"    value in loop, but do remember this will stop loop immediately."               "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    An new list."                                                                  "\n"\
                                                                                    "\n"\
"NOTES"                                                                             "\n"\
"    For `pop_front()` and `pop_back()`, a return value of `nil` does not means"    "\n"\
"    list is empty unless you can sure `nil` is never pushed into list."            "\n"\
                                                                                    "\n"\
"    Always use `size()` to check if list is empty."                                "\n"\
                                                                                    "\n"\
"EXAMPLES"                                                                          "\n"\
"    ```lua"                                                                        "\n"\
"    local list = infra.new_list()"                                                 "\n"\
"    list:push_back(9)"                                                             "\n"\
"    assert(list:size() == 1)"                                                      "\n"\
"    assert(list:pop_front() == 9)"                                                 "\n"\
"    assert(list:size() == 0)"                                                      "\n"\
"    ```"                                                                           "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_NEW_LIST
 */
API_LOCAL int infra_new_list(lua_State* L);

/**
 * @breif Pops a iterator from the stack, and pushes a key–value pair from the
 *   list at the given index, the "next" pair after the given key.
 *
 * If there are no more elements in the list, returns 0 and pushes nothing.
 *
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @return          0 if no more element, 1 if success.
 */
API_LOCAL int infra_list_next(lua_State* L, int idx);

#ifdef __cplusplus
}
#endif

#endif
