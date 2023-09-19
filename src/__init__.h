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

#if defined(_WIN32)
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct infra_lua_api
{
    const char*         name;       /**< Function name. */
    lua_CFunction       addr;       /**< Function initialize entry. */
    int                 upvalue;    /**< Whether require upvalue. */
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
 * @brief Translate system error to standard error.
 * @param[in] errcode   For windows, it must comes from GetLastError(). For unix, it must comes from errno.
 */
API_LOCAL int infra_translate_sys_error(int errcode);

/**
 * @brief Push error string on top of stack \p L.
 * @param[in] L     Lua VM.
 * @param[in] errcode   The error code from GetLastError().
 */
API_LOCAL void infra_push_error(lua_State* L, int errcode);

/**
 * @brief Compat for Windows and Unix
 * @{
 */
#if defined(_MSC_VER)

#define strtok_r(str, delim, saveptr)   strtok_s(str, delim, saveptr)
#define strerror_r(errcode, buf, len)   strerror_s(buf,len, errcode)
#define strdup(s)                       _strdup(s)
#define strcasecmp(s1, s2)              _stricmp(s1, s2)
#define sscanf(b, f, ...)               sscanf_s(b, f, ##__VA_ARGS__)

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

#else

/**
 * @brief Opens a file.
 * @see fopen()
 * @param[out] pFile    A pointer to the file pointer that will receive the
 *   pointer to the opened file.
 * @param[in] filename  Filename.
 * @param[in] mode      Type of access permitted.
 * @return              Zero if successful; an error code on failure.
 */
API_LOCAL int fopen_s(FILE** pFile, const char* filename, const char* mode);

#endif
/**
 * @}
 */

/**
 * @brief Compat for Lua5.x
 */

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUA_ABSINDEX
#define lua_absindex(L, idx)            infra_lua_absindex(L, idx)
int infra_lua_absindex(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_LEN
#define luaL_len(L, idx)                infra_luaL_len(L, idx)
lua_Integer infra_luaL_len(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETFIELD
#define lua_getfield(L, idx, k)         infra_lua_getfield(L, idx, k)
int infra_lua_getfield(lua_State* L, int idx, const char* k);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETTABLE
#define lua_gettable(L, idx)            infra_lua_gettable(L, idx)
int infra_lua_gettable(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETI
#define lua_geti(L, idx, n)             infra_lua_geti(L, idx, n)
int infra_lua_geti(lua_State* L, int idx, lua_Integer n);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_SETI
#define lua_seti(L, idx, n)             infra_lua_seti(L, idx, n)
void infra_lua_seti(lua_State* L, int idx, lua_Integer n);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_SETFUNCS
#define luaL_setfuncs(L, l, nup)        infra_luaL_setfuncs(L, l, nup)
void infra_luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUA_TONUMBERX
#define lua_tonumberx(L, idx, isnum)    infra_lua_tonumberx(L, idx, isnum)
lua_Number infra_lua_tonumberx(lua_State* L, int idx, int* isnum);
#endif

#if !defined(luaL_newlib) && LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_NEWLIB
#define luaL_newlib(L, l)           infra_luaL_newlib(L, l)
void infra_luaL_newlib(lua_State* L, const luaL_Reg l[]);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
