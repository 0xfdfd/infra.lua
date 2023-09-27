#include "__init__.h"

static int _infra_readfile(lua_State* L)
{
    char buf[128];
    const char* path = luaL_checkstring(L, 1);

    FILE* file = NULL;
    int ret = fopen_s(&file, path, "rb");
    if (ret != 0)
    {
        ret = infra_translate_sys_error(ret);
        return infra_raise_error(L, ret);
    }

    lua_pushstring(L, "");

    while (1)
    {
        size_t read_sz = fread(buf, 1, sizeof(buf), file);
        if (read_sz == 0)
        {
            break;
        }

        lua_pushlstring(L, buf, read_sz);
        lua_concat(L, 2);
    }
    fclose(file);
    
    return 1;
}

const infra_lua_api_t infra_f_readfile = {
"readfile", _infra_readfile, 0,
"Read file",

"[SYNOPSIS]\n"
"string readfile(string path)\n"
"\n"
"[DESCRIPTION]\n"
"Read file and return it's content.\n"
};
