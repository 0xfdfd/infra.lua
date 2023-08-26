#include "__init__.h"

static int _split_line(lua_State* L)
{
    size_t data_sz = 0;
    const char* data = luaL_checklstring(L, 1, &data_sz);

    // This will be the return value.
    lua_newtable(L);

    size_t pos;
    const char* start = data;

    for (pos = 0; pos < data_sz; pos++)
    {
        /*
         * There are 3 EOLs:
         * \r\n: CRLF (Windows)
         * \r: CR (Macintosh)
         * \n: LF (Unix)
         * Here we allow mixed line endings, just to parse aggressively.
         */
        if (data[pos] == '\r')
        {
            size_t len = &data[pos] - start;
            lua_pushlstring(L, start, len);
            lua_seti(L, -2, luaL_len(L, -2) + 1);

            if (pos < data_sz - 1 && data[pos + 1] == '\n')
            {
                start = &data[pos + 2];
                pos++;
            }
            else
            {
                start = &data[pos + 1];
            }
            continue;
        }
        else if (data[pos] == '\n')
        {
            size_t len = &data[pos] - start;
            lua_pushlstring(L, start, len);
            lua_seti(L, -2, luaL_len(L, -2) + 1);

            start = &data[pos + 1];
            continue;
        }
    }

    if (start < data + pos)
    {
        size_t len = data + data_sz - start;
        lua_pushlstring(L, start, len);
        lua_seti(L, -2, luaL_len(L, -2) + 1);
    }

    return 1;
}

const infra_lua_api_t infra_f_split_line = {
"split_line", _split_line, 0,
"Split string into array.",

"[SYNOPSIS]\n"
"table split_line(string s)\n"
"\n"
"[DESCRIPTION]\n"
"Split string into array, with line wrap as token. This function automatically\n"
"take care of different endings like `\\r`, `\\n` or `\\r\\n`. Empty line is\n"
"not ignored.\n"
};
