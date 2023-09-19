#ifndef __INFRA_LUA_H__
#define __INFRA_LUA_H__

#if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(INFRA_LUA_EXPOSE_SYMBOLS)
#       if defined(__GNUC__) || defined(__clang__)
#           define INFRA_API    __attribute__ ((dllexport))
#       else
#           define INFRA_API    __declspec(dllexport)
#       endif
#   elif defined(INFRA_LUA_DLL_IMPORT)
#       if defined(__GNUC__) || defined(__clang__)
#           define INFRA_API    __attribute__ ((dllimport))
#       else
#           define INFRA_API    __declspec(dllimport)
#       endif
#   else
#       define INFRA_API
#   endif
#elif (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
#   define INFRA_API __attribute__((visibility ("default")))
#else
#   define INFRA_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

/**
 * @defgroup INFRA infra
 *
 * A library to enhance lua capability.
 *
 * @{
 */

/**
 * @brief Entrypoint for infra.
 *
 * Use `require('infra')` to load infra library.
 *
 * @param[in] L Lua VM.
 * @return      Always 1.
 */
INFRA_API int luaopen_infra(struct lua_State* L);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
