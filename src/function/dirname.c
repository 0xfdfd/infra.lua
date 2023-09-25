#include "__init__.h"

static char* _am_dirname(char* s)
{
    size_t i;
    if (!s || !*s) return ".";
    i = strlen(s) - 1;
    for (; s[i] == '/'; i--) if (!i) return "/";
    for (; s[i] != '/'; i--) if (!i) return ".";
    for (; s[i] == '/'; i--) if (!i) return "/";
    s[i + 1] = 0;
    return s;
}

static int _infra_dirname(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    char* cpy_path = strdup(path);

    char* name = _am_dirname(cpy_path);
    lua_pushstring(L, name);
    free(cpy_path);

    return 1;
}

const infra_lua_api_t infra_f_dirname = {
"dirname", _infra_dirname, 0,
"Break string `s` into directory component and return it.",

"[SYNOPSIS]\n"
"string dirname(string s)\n"
"\n"
"[DESCRIPTION]\n"
"dirname() returns the string up to, but not including, the final '/'. If path\n"
"does not contain a slash, dirname() returns the string \".\".\n"
};
