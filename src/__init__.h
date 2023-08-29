#ifndef __LUA_INFRA_API_H__
#define __LUA_INFRA_API_H__

///////////////////////////////////////////////////////////////////////////////
// HEADER FILES
///////////////////////////////////////////////////////////////////////////////

#define INFRA_LUA_EXPOSE_SYMBOLS
#include <infra.lua.h>

///////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get the number of element in array.
 * @param[in] a Array.
 * @return      Element count.
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @brief Default as local API.
 *
 * On gcc/clang, non-static C function are export by default. However most of
 * library function should not be exported.
 *
 * Use this macro to ensure these functions is hidden for user.
 */
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32)
#   define API_LOCAL    __attribute__((visibility ("hidden")))
#else
#   define API_LOCAL
#endif

#if !defined(offsetof)
#   define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#if !defined(container_of)
#   if defined(__GNUC__) || defined(__clang__)
#       define container_of(ptr, type, member)   \
            ({ \
                const typeof(((type *)0)->member)*__mptr = (ptr); \
                (type *)((char *)__mptr - offsetof(type, member)); \
            })
#   else
#       define container_of(ptr, type, member)   \
            ((type *) ((char *) (ptr) - offsetof(type, member)))
#   endif
#endif

#define CHECK_OOM(L, X) \
    if (NULL == (X)) {\
        return luaL_error(L, "out of memory");\
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

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Initialize callback.
 * @warning Do not break stack balance.
 * @param[in] L     Host Lua VM.
 * @param[out] addr Function address.
 * @return          The number of upvalue.
 */
typedef int (*infra_api_init_fn)(lua_State* L, lua_CFunction* addr);

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
extern const infra_lua_api_t infra_f_compare;
extern const infra_lua_api_t infra_f_dirname;
extern const infra_lua_api_t infra_f_dump_any;
extern const infra_lua_api_t infra_f_dump_hex;
extern const infra_lua_api_t infra_f_man;
extern const infra_lua_api_t infra_f_merge_line;
extern const infra_lua_api_t infra_f_split_line;
extern const infra_lua_api_t infra_f_strcasecmp;
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
 * @brief Compat for Windows and Unix
 * @{
 */
#if defined(_MSC_VER)

#define strtok_r(str, delim, saveptr)   strtok_s(str, delim, saveptr)
#define strerror_r(errcode, buf, len)   strerror_s(buf,len, errcode)
#define strdup(s)                       _strdup(s)
#define strcasecmp(s1, s2)              _stricmp(s1, s2)

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
int fopen_s(FILE** pFile, const char* filename, const char* mode);

#endif
/**
 * @}
 */

/**
 * @brief Compat for Lua5.x
 */

#if LUA_VERSION_NUM <= 501
#define lua_absindex(L, idx)    infra_lua_absindex(L, idx)
int infra_lua_absindex(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 501
#define luaL_len(L, idx)        infra_luaL_len(L, idx)
lua_Integer infra_luaL_len(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 502
#define lua_geti(L, idx, n)     infra_lua_geti(L, idx, n)
int infra_lua_geti(lua_State* L, int idx, lua_Integer n);
#endif

#if LUA_VERSION_NUM <= 502
#define lua_seti(L, idx, n)     infra_lua_seti(L, idx, n)
void infra_lua_seti(lua_State* L, int idx, lua_Integer n);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
