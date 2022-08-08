/**
 * @file
 */
#ifndef __INFRA_LUA_API_MAP_H__
#define __INFRA_LUA_API_MAP_H__

#include <infra.lua/defs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFRA_LUA_API_NEW_MAP(XX)   \
XX(                                                                                     \
"new_map", infra_new_map, NULL,                                                         \
"Create a new list map.",                                                               \
"SYNOPSIS"                                                                          "\n"\
"    map new_map();"                                                                "\n"\
"    map new_map(table);"                                                           "\n"\
"    map new_map(map);"                                                             "\n"\
"    map new_map(__pairs);"                                                         "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `new_map()` function create a map."                                        "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    A new map."                                                                    "\n"\
)

/**
 * @defgroup INFRA_MAP map
 * Checkout #infra_new_map() for lua methods.
 * @{
 */

/**
 * @brief Create a new map and push it onto top of stack \p L.
 *
 * The map support following method:
 *
 * + `map:assign(key, value)`
 *
 *   Insert key-value pair into map. If key exists, the value replace the
 *   original one. Return a iterator point to this key-value pair.
 *
 * + `map:begin()`
 *
 *   Get a iterator point to begin of map.
 *
 * + `map:clear()`
 *
 *   Clear contents.
 *
 * + `map:clone()`
 *
 *   Clone self and return a new map. Modify the new map will not affect the
 *   original one. But note that key and value does not copied.
 *
 * + `map:copy(table)`
 *
 *   Copy all key-value pair from table into map. The `table` can be a lua
 *   table, a infra map, or anything with `__pairs` metamethod.
 *
 * + `map:equal(table)`
 *
 *   Compare with `table`. A table can be a lua table, a infra map, or any
 *   object have `__pairs` and `__index` metamethod.
 *
 * + `map:erase(iter)`
 *
 *   Erase a map node which was point by `iter`.
 *
 * + `iter map:insert(key, value)`
 *
 *   Insert key-value pair into map. If key exists, the insert operation fails
 *   and return nil. Return a iterator point to this key-value pair.
 *
 * + `map:next(iter)`
 *
 *   Get a iterator next to `iter`.
 *
 * + `map:size()`
 *
 *   Get the number of key-value pair in this map.
 *
 * The map support following metamethod:
 * + `__add`
 *
 *   Union of Sets. Return a map contains all key-value pair from first operand
 *   and second operand.
 *
 *   If the same key exists in both first operand and second operand, the value
 *   is taken from the first operand.
 *
 *   ```lua
 *   a = infra.new_map({"hello", "to"})
 *   b = infra.new_map({"hello", "world"})
 *   c = a + b
 *   assert(c == { "hello", "to", "world" })
 *   ```
 *
 * + `__band`
 *
 *   Intersection of Sets. Return a map contains shared key-value pair from
 *   first operand and second operand.
 *
 *   The value comes from first operand.
 *
 * + `__bor`
 *
 *   The same as metamethod `__add`.
 *
 * + `__eq`
 *
 *   Compare two infra map. Return true if all key-value pair are equal,
 *   otherwise return false.
 *
 *   The second operand can be a infra map, or any object have `__pairs` and
 *   `__index` metamethod.
 *
 *   Due to lua limitation, the second operand can not be lua table. Use
 *   `map:equal()` to do such compare.
 *
 * + `__gc`
 *
 *   Life cycle is controlled by lua vm.
 *
 * + `__index`
 *
 *   Syntax `v = t[k]` is allowed.
 *
 * + `__len`
 *
 *   The same as `map:size()`.
 *
 * + `__newindex`
 *
 *   Syntax `t[k] = v` is allowed.
 *
 *   Note that unlike lua table, `t[k] = nil` does not delete record, it only
 *   replace value with `nil`. Use `map:erase()` to completely delete a
 *   key-value pair.
 *
 * + `__pairs`
 *
 *   Syntax `for k,v in pairs(t) do body end` is allowed.
 *
 * + `__pow`
 *
 *   Complement of Sets.
 *
 * + `__sub`
 *
 *
 * @param[in] L     Lua VM.
 * @return          1 if success, 0 if failure.
 */
INFRA_API int infra_new_map(lua_State* L);

/**
 * @brief Push empty map on top of stack.
 * @param[out] L    Lua VM.
 * @return          Always 1.
 */
INFRA_API int infra_push_empty_map(lua_State* L);

/**
 * @brief Compare infra map at \p idx1 with table at \p idx2.
 * @param[in] L     Lua VM.
 * @param[in] idx1  Index for infra map.
 * @param[in] idx2  Index for table. It can be a lua table, a infra table, or
 *   any object have `__pairs` and `__index` metamethod.
 * @return          1 if equal, 0 if not.
 */
INFRA_API int infra_map_compare(lua_State* L, int idx1, int idx2);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
