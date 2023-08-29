#include <infra.lua.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "test.h"

typedef struct loader_arg
{
    const char* name;
    const char* script;
} loader_arg_t;

typedef struct test_ctx
{
    lua_State* L;
} test_ctx_t;

test_ctx_t g_test_ctx = { NULL };

static void _close_lua_ctx(void)
{
    if (g_test_ctx.L != NULL)
    {
        lua_close(g_test_ctx.L);
        g_test_ctx.L = NULL;
    }
}

static void _on_before_test(const char* fixture, const char* test_name)
{
    (void)fixture; (void)test_name;
    _close_lua_ctx();

    g_test_ctx.L = luaL_newstate();
    assert(g_test_ctx.L != NULL);
}

static void _on_after_test(const char* fixture, const char* test_name, int ret)
{
    (void)fixture; (void)test_name; (void)ret;
    _close_lua_ctx();
}

static void _on_after_all_test(void)
{
    _close_lua_ctx();
}

static int _is_eq(lua_State* L)
{
    if (lua_rawequal(L, 1, 2))
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    if (luaL_getmetafield(L, 1, "__eq") != LUA_TNIL)
    {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            goto custom_compare;
        }
        lua_pop(L, 1);
    }

    if (luaL_getmetafield(L, 2, "__eq") != LUA_TNIL)
    {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            goto custom_compare;
        }
        lua_pop(L, 1);
    }

    lua_pushboolean(L, 0);
    return 1;

custom_compare:
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);
    return 1;
}

static int _assert_eq(lua_State* L)
{
    int sp = lua_gettop(L);

    lua_pushcfunction(L, _is_eq);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1); // sp+1

    if (lua_toboolean(L, -1))
    {
        return 0;
    }
    lua_pop(L, 1);

    /* sp+1 */
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    /* sp+2 */
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);

    /* sp+3 */
    if (sp >= 3)
    {
		lua_getglobal(L, "string");
		lua_getfield(L, -1, "format");
		lua_remove(L, sp + 3);
        int i = 3;
        for (; i <= sp; i++)
        {
            lua_pushvalue(L, i);
        }
        lua_call(L, sp - 2, 1);
    }
    else
    {
        lua_pushstring(L, "");
    }

    lua_pushfstring(L, "Assertion failed: %s: `%s` == `%s`.",
        lua_tostring(L, sp + 3), lua_tostring(L, sp + 1), lua_tostring(L, sp + 2));
    return lua_error(L);
}

static int _assert_ne(lua_State* L)
{
    lua_pushcfunction(L, _is_eq);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);

    if (lua_toboolean(L, -1) == 0)
    {
        return 0;
    }
    lua_settop(L, 2);

    /* sp:3 */
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    /* sp:3 */
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);

    lua_pushfstring(L, "Assertion failed: `%s` != `%s`.",
        lua_tostring(L, 3), lua_tostring(L, 4));
    return lua_error(L);
}

static int _luaopen_test(lua_State* L)
{
    static const luaL_Reg s_api[] = {
        { "is_eq",      _is_eq },
        { "assert_eq",  _assert_eq },
        { "assert_ne",  _assert_ne },
        { NULL,         NULL },
    };
#if defined(luaL_newlib)
    luaL_newlib(L, s_api);
#else
    lua_newtable(L);
    size_t i;
    for (i = 0; s_api[i].name != NULL; i++)
    {
        lua_pushcfunction(L, s_api[i].func);
        lua_setfield(L, -2, s_api[i].name);
    }
#endif

    return 1;
}

#if LUA_VERSION_NUM <= 501
void luaL_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb)
{
    int sp = lua_gettop(L);

    /*
     * sp+1: the mod
     */
    lua_pushcfunction(L, openf);
    lua_call(L, 0, 1);

    if (glb)
    {
        lua_pushvalue(L, sp + 1);
        lua_setglobal(L, modname);
    }

    lua_getglobal(L, "package"); // sp+2: package
    lua_getfield(L, -1, "loaded"); // sp+3: loaded

    /* Save mod */
    lua_pushvalue(L, sp + 1);
    lua_setfield(L, sp + 3, modname);

    /* Update loaded */
    lua_setfield(L, sp + 2, "loaded"); // sp+2

    /* Update package */
    lua_setglobal(L, "package"); // sp+1

    lua_settop(L, sp);
}
#endif

static int _test_loader(lua_State* L)
{
    loader_arg_t* arg = lua_touserdata(L, 1);

    luaL_openlibs(L);
    luaL_requiref(L, "infra", luaopen_infra, 1);
    luaL_requiref(L, "test", _luaopen_test, 1);

    if (luaL_loadbuffer(L, arg->script, strlen(arg->script), arg->name) != 0)
    {
        return lua_error(L);
    }

    lua_call(L, 0, 0);
    return 0;
}

#if LUA_VERSION_NUM >= 502
static int _msg_handler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (msg == NULL)
    {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
        {
            return 1;  /* that is the message */
        }

        msg = lua_pushfstring(L, "(error object is a %s value)",
            luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}
#endif

void test_run_script(const char* name, const char* script)
{
    loader_arg_t arg = { name, script };

#if LUA_VERSION_NUM >= 502
    int sp = lua_gettop(g_test_ctx.L);
    lua_pushcfunction(g_test_ctx.L, _msg_handler);
    lua_pushcfunction(g_test_ctx.L, _test_loader);
    lua_pushlightuserdata(g_test_ctx.L, &arg);
    int script_result = lua_pcall(g_test_ctx.L, 1, 0, sp + 1);
#else
    int script_result = lua_cpcall(g_test_ctx.L, _test_loader, &arg);
#endif

    ASSERT_EQ_INT(script_result, 0, "%s", lua_tostring(g_test_ctx.L, -1));
}

int main(int argc, char* argv[])
{
    static cutest_hook_t hook;
    memset(&hook, 0, sizeof(hook));

    hook.before_test = _on_before_test;
    hook.after_test = _on_after_test;
    hook.after_all_test = _on_after_all_test;

    return cutest_run_tests(argc, argv, stdout, &hook);
}
