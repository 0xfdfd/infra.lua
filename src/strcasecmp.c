#include "__init__.h"

static int _infra_strcasecmp(lua_State* L)
{
    const char* s1 = luaL_checkstring(L, 1);
    const char* s2 = luaL_checkstring(L, 2);

    int ret = strcasecmp(s1, s2);
    lua_pushinteger(L, ret);

    return 1;
}

const infra_lua_api_t infra_f_strcasecmp = {
"strcasecmp", _infra_strcasecmp, 0,
"Compare string ignoring the case of the characters.",

"[SYNOPSIS]\n"
"number strcasecmp(string s1, string s2)\n"
"\n"
"[DESCRIPTION]\n"
"The strcasecmp() function performs a byte-by-byte comparison of the strings `s1`\n"
"and `s2`, ignoring the case of the characters. It returns an integer less than,\n"
"equal to, or greater than zero if `s1` is found, respectively, to be less than,\n"
"to match, or be greater than `s2`.\n"
};
