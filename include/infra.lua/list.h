#ifndef __INFRA_LUA_LIST_H__
#define __INFRA_LUA_LIST_H__

#include <infra.lua/defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup INFRA_LIST list
 *
 * @section INFRA_LIST_SYNOPSIS 1. SYNOPSIS
 *
 * ```lua
 * list new_list();
 * list new_list(table);
 * ```
 *
 * @section INFRA_LIST_DESCRIPTION 2. DESCRIPTION
 *
 * The `new_list()` create a new list and return. If argument is a lua native
 * list, the content is copied in order.
 *
 * @subsection INFRA_LIST_DESCRIPTION_METHOD 2.1. METHOD
 *
 * The list have following method:
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_SIZE size
 *
 * ```lua
 * integer list:size()
 * ```
 *
 * Get the number of element in list.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_PUSH_FRONT push_front
 *
 * ```lua
 * iter list:push_front(v)
 * ```
 *
 * Push value into front of list and return iterator.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_PUSH_BACK push_back
 *
 * ```lua
 * iter list:push_back(v)
 * ```
 *
 * Push value into end of list and return iterator.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP_FRONT pop_front
 *
 * ```lua
 * v list:pop_front()
 * ```
 *
 * Pop front value of list and return it.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP_BACK pop_back
 *
 * ```lua
 * v list:pop_back
 * ```
 *
 * Pop end value of list and return it.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP_ERASE erase
 *
 * ```lua
 * void list:erase(iter)
 * ```
 *
 * Erase value at iterator.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP_BEGIN begin
 *
 * ```lua
 * iter list:begin()
 * ```
 *
 * Get iterator point to begin of list. A return value of `nil` means list is
 * empty.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP_NEXT next
 *
 * ```lua
 * iter list:next(iter)
 * ```
 *
 * Get next iterator. A return value of `nil` means no more element in list.
 *
 * @subsection INFRA_LIST_DESCRIPTION_META_METHOD 2.2. META METHOD
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP___GC __gc
 *
 * Life cycle is controlled by lua vm.
 *
 * @subsubsection INFRA_LIST_DESCRIPTION_POP___pairs __pairs
 *
 * The following syntax is allowed:
 *
 * ```lua
 * for k,v in pairs(t) do body end
 * ```
 *
 * The k in loop is the internal index of value. You are safe to erase this
 * value in loop, but do remember this will stop loop immediately.
 *
 * For example:
 *
 * ```lua
 * local list = infra.new_list({"hello", "world"})
 * for k, v in pairs(list) do
 *     list:erase(k)
 * end
 * assert(list:size() == 1)
 * assert(list:pop_front() == "world")
 * ```
 *
 * @section INFRA_LIST_RETURN 3. RETURN VALUE
 *
 * An new list.
 *
 * @section INFRA_LIST_NOTES 4. NOTES
 *
 * For `pop_front()` and `pop_back()`, a return value of `nil` does not means
 * list is empty unless you can sure `nil` is never pushed into list.
 *
 * Always use `size()` to check if list is empty.
 *
 * @section INFRA_LIST_EXAMPLES 5. EXAMPLES
 *
 * ```lua
 * local list = infra.new_list()
 * list:push_back(9)
 * assert(list:size() == 1)
 * assert(list:pop_front() == 9)
 * assert(list:size() == 0)
 * ```
 *
 * @{
 */

/**
 * @brief Pops a iterator from the stack, and pushes a key–value pair from the
 *   list at the given index, the "next" pair after the given key.
 *
 * If there are no more elements in the list, returns 0 and pushes nothing.
 *
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @return          0 if no more element, 1 if success.
 */
INFRA_API int infra_list_next(lua_State* L, int idx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
