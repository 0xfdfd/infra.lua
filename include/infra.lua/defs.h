#ifndef __INFRA_LUA_DEFINES_H__
#define __INFRA_LUA_DEFINES_H__

#if defined(_WIN32)
#   define INFRA_API    __declspec(dllexport)
#else
#   define INFRA_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
}
#endif

#endif
