#ifndef __LUA_INFRA_API_H__
#define __LUA_INFRA_API_H__

///////////////////////////////////////////////////////////////////////////////
// HEADER FILES
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// Application Programming Interface
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize callback.
 * @warning Do not break stack balance.
 * @param[in] L     Host Lua VM.
 */
typedef void (*infra_lua_init_fn)(lua_State* L);

typedef struct infra_lua_api
{
    const char*         name;       /**< Function name */
    lua_CFunction       func;       /**< Function address */
    infra_lua_init_fn   init;       /**< Initialize function */
    const char*         brief;      /**< Brief document. */
    const char*         document;   /**< Detail document. */
} infra_lua_api_t;

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

#ifdef __cplusplus
}
#endif

#endif
