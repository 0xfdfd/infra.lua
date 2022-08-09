#ifndef __INFRA_LUA_MAP_H__
#define __INFRA_LUA_MAP_H__

#include <infra.lua/defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup INFRA_MAP map
 *
 * @section INFRA_MAP_SYNOPSIS 1. SYNOPSIS
 *
 * ```lua
 * map infra.new_map();
 * map infra.new_map(table);
 * ```
 *
 * @section INFRA_MAP_DESCRIPTION 2. DESCRIPTION
 *
 * Create a new empty infra map.
 *
 * To create a map with key-value pair, pass a table to it. The parameter can
 * be a lua table, a infra map, or any object that has `__pairs` metamethod.
 *
 * @subsection INFRA_MAP_DESCRIPTION_METHOD 2.1. METHOD
 *
 * The map support following method:
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_ASSIGN assign
 *
 * ```lua
 * iter map:assign(key, value)
 * ```
 *
 * Insert key-value pair into map. If key exists, the value replace the
 * original one. Return a iterator point to this key-value pair.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_BEGIN begin
 *
 * ```lua
 * iter map:begin()
 * ```
 *
 * Get a iterator point to begin of map.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_CLEAR clear
 *
 * ```lua
 * void map:clear()
 * ```
 *
 * Clear contents.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_CLONE clone
 *
 * ```lua
 * map map:clone()
 * ```
 *
 * Clone self and return a new map. Modify the new map will not affect the
 * original one. But note that key and value does not copied.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_COPY copy
 *
 * ```lua
 * void map:copy(table, force)
 * ```
 *
 * Copy all key-value pair from table into map. The `table` can be a lua
 * table, a infra map, or anything with `__pairs` metamethod.
 *
 * If key already exist in table, the value is not updated. To overwrite, set
 * `force` to true.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_EQUAL equal
 *
 * ```lua
 * boolean map:equal(table)
 * ```
 *
 * Compare with `table`. A table can be a lua table, a infra map, or any
 * object have `__pairs` and `__index` metamethod.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_ERASE erase
 *
 * ```lua
 * void map:erase(iter)
 * ```
 *
 * Erase a map node which was point by `iter`.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_INSERT insert
 *
 * ```lua
 * iter map:insert(key, value)
 * ```
 *
 * Insert key-value pair into map. If key exists, the insert operation fails
 * and return nil. Return a iterator point to this key-value pair.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_NEXT next
 *
 * ```lua
 * iter map:next(iter)
 * ```
 *
 * Get a iterator next to `iter`.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_SIZE size
 *
 * ```lua
 * integer map:size()
 * ```
 *
 * Get the number of key-value pair in this map.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION_V v
 *
 * ```lua
 * visitor map:v()
 * ```
 *
 * Get visitor for this map. You can use visitor to access key-value directly.
 *
 * Note that unlike lua table, `t[k] = nil` does not delete record, it only
 * replace value with `nil`. Use @ref INFRA_MAP_DESCRIPTION_ERASE to completely
 * delete a key-value pair.
 *
 * For example:
 *
 * ```lua
 * local t = infra.new_map()
 * t:v()["hello"] = "world"
 * assert(t:v()["hello"] == "world")
 * ```
 *
 * @subsection INFRA_MAP_DESCRIPTION_META_METHOD 2.2. META METHOD
 *
 * The map support following meta method:
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___ADD __add
 *
 * Union of Sets. Return a map contains all key-value pair from first operand
 * and second operand.
 *
 * If the same key exists in both first operand and second operand, the value
 * is taken from the first operand.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___BAND __band
 *
 * Intersection of Sets. Return a map contains shared key-value pair from
 * first operand and second operand.
 *
 * The value comes from first operand.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___BOR __bor
 *
 * The same as meta method @ref INFRA_MAP_DESCRIPTION___ADD.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___EQ __eq
 *
 * Compare two infra map. Return true if all key-value pair are equal,
 * otherwise return false.
 *
 * The second operand can be a lua table, a infra map, or any object have
 * `__pairs` and `__index` meta method.
 *
 * Note: Due to lua limitation, only when both operand of `==` is a full
 * userdata will trigger this meta method, so use `==` to compare infra map
 * with lua table is just not working.  Either get meta method of `__eq` or use
 * @ref INFRA_MAP_DESCRIPTION_EQUAL to do comparison when in pure lua code.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___GC __gc
 *
 * Life cycle is controlled by lua vm.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___LEN __len
 *
 * The same as @ref INFRA_MAP_DESCRIPTION_SIZE.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___PAIRS __pairs
 *
 * The following syntax is allowed:
 *
 * ```lua
 * for k,v in pairs(t) do body end
 * ```
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___POW __pow
 *
 *   Complement of Sets.
 *
 * @subsubsection INFRA_MAP_DESCRIPTION___SUB __sub
 *
 *   Set Difference.
 *
 * @section INFRA_MAP_RETURN 3. RETURN VALUE
 *
 * A new infra map.
 *
 * @{
 */

/**
 * @brief Create and push empty map on top of stack.
 * @param[out] L    Lua VM.
 * @return          Always 1.
 */
INFRA_API int infra_map_new(lua_State* L);

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
