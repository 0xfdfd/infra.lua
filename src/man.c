#include "__init__.h"
#include <string.h>

typedef struct api_man_foreach_helper
{
    lua_State*  L;
    const char* name;
    int         ret;
} api_man_foreach_helper_t;

static int _api_man_foreach(const infra_lua_api_t* api, void* arg)
{
    api_man_foreach_helper_t* helper = arg;

    if (strcmp(api->name, helper->name) != 0)
    {
        return 0;
    }

    lua_pushstring(helper->L, api->document);
    helper->ret = 1;

    return 1;
}

static int _api_man(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);

    api_man_foreach_helper_t helper = { L, name, 0 };
    lua_api_foreach(_api_man_foreach, &helper);
    return helper.ret;
}

const infra_lua_api_t infra_f_man = {
"man", _api_man, 0,
"Show manual about function.",

"[SYNOPSIS]\n"
"string man(string name)\n"
"\n"
"[DESCRIPTION]\n"
"Search for manual about function `name`. If found, return the manual as string,\n"
"otherwise return nil.\n",
};
