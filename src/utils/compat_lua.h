/**
 * @brief This file provides compatible functions for Lua version 5.x.
 */
#ifndef __INFRA_LUA_COMPAT_LUA_H__
#define __INFRA_LUA_COMPAT_LUA_H__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/**
 * @brief Compat for Lua5.x
 * @{
 */

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUA_ABSINDEX
#define lua_absindex(L, idx)            infra_lua_absindex(L, idx)
API_LOCAL int infra_lua_absindex(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_LEN
#define luaL_len(L, idx)                infra_luaL_len(L, idx)
API_LOCAL lua_Integer infra_luaL_len(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETFIELD
#define lua_getfield(L, idx, k)         infra_lua_getfield(L, idx, k)
API_LOCAL int infra_lua_getfield(lua_State* L, int idx, const char* k);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETTABLE
#define lua_gettable(L, idx)            infra_lua_gettable(L, idx)
API_LOCAL int infra_lua_gettable(lua_State* L, int idx);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_GETI
#define lua_geti(L, idx, n)             infra_lua_geti(L, idx, n)
API_LOCAL int infra_lua_geti(lua_State* L, int idx, lua_Integer n);
#endif

#if LUA_VERSION_NUM <= 502
#define INFRA_NEED_LUA_SETI
#define lua_seti(L, idx, n)             infra_lua_seti(L, idx, n)
API_LOCAL void infra_lua_seti(lua_State* L, int idx, lua_Integer n);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_SETFUNCS
#define luaL_setfuncs(L, l, nup)        infra_luaL_setfuncs(L, l, nup)
API_LOCAL void infra_luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup);
#endif

#if LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUA_TONUMBERX
#define lua_tonumberx(L, idx, isnum)    infra_lua_tonumberx(L, idx, isnum)
API_LOCAL lua_Number infra_lua_tonumberx(lua_State* L, int idx, int* isnum);
#endif

#if !defined(luaL_newlib) && LUA_VERSION_NUM <= 501
#define INFRA_NEED_LUAL_NEWLIB
#define luaL_newlib(L, l)               infra_luaL_newlib(L, l)
API_LOCAL void infra_luaL_newlib(lua_State* L, const luaL_Reg l[]);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
