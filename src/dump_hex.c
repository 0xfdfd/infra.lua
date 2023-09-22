#include "__init__.h"
#include <stdint.h>

static char _mmc_ascii_to_char(unsigned char c)
{
    if (c >= 32 && c <= 126)
    {
        return c;
    }
    return '.';
}

static void mmc_dump_hex(luaL_Buffer* buf, const void* data, size_t size, size_t width)
{
    char tmp[64];
    const unsigned char* pdat = (unsigned char*)data;

    size_t idx_line;
    for (idx_line = 0; idx_line < size; idx_line += width)
    {
        snprintf(tmp, sizeof(tmp), "0x%08x: ", (unsigned)idx_line);
        luaL_addstring(buf, tmp);

        /* printf hex */
        size_t idx_colume;
        for (idx_colume = 0; idx_colume < width; idx_colume++)
        {
            const char* postfix = (idx_colume < width - 1) ? "" : "|";

            if (idx_colume + idx_line < size)
            {
                snprintf(tmp, sizeof(tmp), "%02x %s", pdat[idx_colume + idx_line], postfix);
            }
            else
            {
                snprintf(tmp, sizeof(tmp), "   %s", postfix);
            }
            luaL_addstring(buf, tmp);
        }
        luaL_addchar(buf, ' ');
        /* printf char */
        for (idx_colume = 0; (idx_colume < width) && (idx_colume + idx_line < size); idx_colume++)
        {
            luaL_addchar(buf, _mmc_ascii_to_char(pdat[idx_colume + idx_line]));
        }
        luaL_addchar(buf, '\n');
    }
}

static int _dump_hex(lua_State* L)
{
    size_t data_sz = 0;
    const char* data = luaL_checklstring(L, 1, &data_sz);

    luaL_Buffer buf;
    luaL_buffinit(L, &buf);
    mmc_dump_hex(&buf, data, data_sz, 16);
    luaL_pushresult(&buf);

    return 1;
}

const infra_lua_api_t infra_f_dump_hex = {
"dump_hex", _dump_hex, 0,
"Dump string as hex string.",

"[SYNOPSIS]\n"
"string dump_hex(string s)\n"
"\n"
"[DESCRIPTION]\n"
"Dump string as hex string with ASCII character shown.\n"
};
