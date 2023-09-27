#include "__init__.h"

static int _infra_writefile(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);

    size_t data_sz = 0;
    const char* data = luaL_checklstring(L, 2, &data_sz);

    const char* mode = luaL_optstring(L, 3, "wb");

    FILE* file = NULL;
    int ret = fopen_s(&file, path, mode);
    if (ret != 0)
    {
        ret = -ret;
        return infra_raise_error(L, ret);
    }

    if (fwrite(data, data_sz, 1, file) != 1)
    {
        abort();
    }

    fclose(file);
    return 0;
}

const infra_lua_api_t infra_f_writefile = {
"writefile", _infra_writefile, 0,
"Write data into file.",

"[SYNOPSIS]\n"
"writefile(string path, string data [, string mode])\n"
"\n"
"[DESCRIPTION]\n"
"Write data into file. By default the `mode` is \"wb\", change it according to\n"
"your need.\n"
};
