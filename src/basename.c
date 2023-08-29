#include "__init__.h"

static const char* _internal_basename(const char* s)
{
    if (!s || !*s)
    {
        return ".";
    }

    const char* pos = s;

    for (; *s; ++s)
    {
        if (*s == '\\' || *s == '/')
        {
            pos = s + 1;
        }
    }
    return pos;
}

static int _infra_basename(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);

    const char* name = _internal_basename(path);
    lua_pushstring(L, name);

    return 1;
}

const infra_lua_api_t infra_f_basename = {
"basename", _infra_basename, 0,
"Strip directory and suffix from filenames.",

"[SYNOPSIS]\n"
"string basename(string s)\n"
"\n"
"[DESCRIPTION]\n"
"Break pathname string into filename component.\n"
};
