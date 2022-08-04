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

/**
 * @brief Raises a type error for the argument arg of the C function that
 *   called it, using a standard message.
 * @note This function never returns.
 * @param[in] L     Lua VM.
 * @param[in] arg   Argument index.
 * @param[in] tname The expected type.
 * @return          Must ignored.
 */
API_LOCAL int infra_typeerror(lua_State *L, int arg, const char *tname);

/**
 * @brief Allocates \p size bytes and returns a pointer to the allocated
 *   memory.
 * This function never fail.
 * @note This memory does not registered into lua vm, you need to free it as
 *   long as don't need it.
 * @param[in] L     Lua VM.
 * @param[in] size  The number of bytes
 * @return          Allocated memory.
 */
API_LOCAL void* infra_malloc(lua_State* L, size_t size);

/**
 * @brief Free memory allocated by #infra_malloc().
 * @param[in] L     Lua VM.
 * @param[in] p     The allocated memory.
 */
API_LOCAL void infra_free(lua_State* L, void* p);

#ifdef __cplusplus
}
#endif

#endif
