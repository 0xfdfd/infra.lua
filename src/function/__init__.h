#ifndef __LUA_INFRA_API_H__
#define __LUA_INFRA_API_H__

///////////////////////////////////////////////////////////////////////////////
// HEADER FILES
///////////////////////////////////////////////////////////////////////////////

#define INFRA_LUA_EXPOSE_SYMBOLS
#include <infra.lua.h>
#include <stdint.h>
#include "utils/defs.h"

///////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////

#define INFRA_LUA_ERRMSG_OOM   "out of memory."

#define INFRA_CHECK_OOM(L, X) \
    if (NULL == (X)) {\
        return luaL_error(L, INFRA_LUA_ERRMSG_OOM);\
    }

/**
 * @brief This macro can help programmers find bugs in their programs, or
 *   handle exceptional cases via a crash that will produce limited debugging
 *   output.
 * @param[in] L     Lua VM.
 * @param[in] x     expression.
 */
#define INFRA_ASSERT(L, x)  \
    ((x) ? 0 : luaL_error(L, "%s:%d: %s: Assertion `%s` failed.", __FILE__, __LINE__, __FUNCTION__, #x))

///////////////////////////////////////////////////////////////////////////////
// Application Programming Interface
///////////////////////////////////////////////////////////////////////////////

#include "utils/compat_lua.h"
#include "utils/compat_sys.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct infra_lua_api
{
    const char*         name;       /**< Function name. */
    lua_CFunction       addr;       /**< Function initialize entry. */

    /**
     * @brief Whether require upvalue.
     *
     * If set, get upvalue by `lua_getupvalue(L, 1)`.
     * The KEY must be prefixed with `__u_`.
     *
     * @warning Never access infra C API which need upvalue.
     */
    int                 upvalue;

    const char*         brief;      /**< Brief document. */
    const char*         document;   /**< Detail document. */
} infra_lua_api_t;

/**
 * @brief This is the list of builtin APIs.
 * @{
 */
extern const infra_lua_api_t infra_f_argparser;
extern const infra_lua_api_t infra_f_basename;
extern const infra_lua_api_t infra_f_compare;
extern const infra_lua_api_t infra_f_cwd;
extern const infra_lua_api_t infra_f_dirname;
extern const infra_lua_api_t infra_f_dump_any;
extern const infra_lua_api_t infra_f_dump_hex;
extern const infra_lua_api_t infra_f_execute;
extern const infra_lua_api_t infra_f_exepath;
extern const infra_lua_api_t infra_f_man;
extern const infra_lua_api_t infra_f_map;
extern const infra_lua_api_t infra_f_merge_line;
extern const infra_lua_api_t infra_f_pairs;
extern const infra_lua_api_t infra_f_range;
extern const infra_lua_api_t infra_f_readdir;
extern const infra_lua_api_t infra_f_readfile;
extern const infra_lua_api_t infra_f_split_line;
extern const infra_lua_api_t infra_f_strcasecmp;
extern const infra_lua_api_t infra_f_writefile;
/**
 * @}
 */

/**
 * @breif API information callback.
 * @param[in] api   API information.
 * @param[in] arg   User defined argument.
 * @return          0 to continue, otherwise stop.
 */
typedef int (*lua_api_foreach_fn)(const infra_lua_api_t* api, void* arg);

/**
 * @brief Get all API information.
 * @param fn[in]    callback.
 * @param arg[in]   User defined argument.
 */
API_LOCAL void lua_api_foreach(lua_api_foreach_fn cb, void* arg);

/**
 * @brief Raises a type error for the argument arg of the C function that
 *   called it, using a standard message.
 * @note This function never returns.
 * @param[in] L     Lua VM.
 * @param[in] arg   Argument index.
 * @param[in] tname The expected type.
 * @return          Must ignored.
 */
API_LOCAL int infra_typeerror(lua_State* L, int arg, const char *tname);

/**
 * @brief Make a tempory buffer on top of Lua stack.
 * @param[in] L     Lua VM.
 * @param[in] size  Buffer size.
 * @return          Buffer address. It always align to twice of machine size.
 */
API_LOCAL void* infra_tmpbuf(lua_State* L, size_t size);

/**
 * @brief Push error string on top of stack \p L.
 * @param[in] L     Lua VM.
 * @param[in] errcode   The error code from GetLastError().
 */
API_LOCAL void infra_push_error(lua_State* L, int errcode);

API_LOCAL int infra_raise_error(lua_State* L, int errcode);

/**
 * @brief Compat for Windows and Unix
 * @{
 */
#if defined(_MSC_VER)

/**
 * @brief Maps a UTF-16 (wide character) string to a new character string.
 * @param[in] str   Wide character string.
 * @return          UTF-8 string. Do remember to free() it.
 */
API_LOCAL char* infra_wide_to_utf8(const WCHAR* str);

/**
 * @brief Maps a UTF-16 (wide character) string to a new character string.
 * @param[in] str   Wide character string.
 * @return          UTF-8 string. Don't free() it, as Lua take care of
 *                  the life cycle.
 */
API_LOCAL char* infra_lua_wide_to_utf8(lua_State* L, const WCHAR* str);

/**
 * @brief Maps a character string to a UTF-16 (wide character) string.
 * @param[in] str   UTF-8 string.
 * @return          Wide character string. Do remember to free() it.
 */
API_LOCAL WCHAR* infra_utf8_to_wide(const char* str);

/**
 * @brief Maps a character string to a UTF-16 (wide character) string, push the
 *   result on top of stack, and return it's address.
 * @param[in] str   UTF-8 string.
 * @return          Wide character string. Don't free() it, as Lua take care of
 *                  the life cycle.
 */
API_LOCAL WCHAR* infra_lua_utf8_to_wide(lua_State* L, const char* str);

#endif
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
