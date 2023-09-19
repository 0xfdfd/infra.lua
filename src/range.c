#include "__init__.h"

#define INFRA_RANGE_NAME    "__infra_range"

typedef struct infra_range
{
    lua_Integer start;
    lua_Integer stop;
    lua_Integer step;
} infra_range_t;

static int _infra_range_index(lua_State* L)
{
    infra_range_t* self = luaL_checkudata(L, 1, INFRA_RANGE_NAME);

    lua_Integer n = luaL_checkinteger(L, 2);
    if (n < self->start || n >= self->stop)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    int fill = ((n - self->start) % self->step) == 0;
    lua_pushboolean(L, fill);
    return 1;
}

static int _infra_range_next(lua_State* L)
{
    infra_range_t* self = luaL_checkudata(L, 1, INFRA_RANGE_NAME);
    if (lua_type(L, 2) == LUA_TNIL)
    {
        lua_pushinteger(L, self->start);
        return 1;
    }

    lua_Integer n = luaL_checkinteger(L, 2);
    n += self->step;

    if (n >= self->stop)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, n);
    return 1;
}

static int _infra_range_pairs(lua_State* L)
{
    lua_pushcfunction(L, _infra_range_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int _infra_range(lua_State* L)
{
    infra_range_t* self = lua_newuserdata(L, sizeof(infra_range_t));

    self->start = luaL_checkinteger(L, 1);
    self->stop = luaL_checkinteger(L, 2);
    self->step = luaL_optinteger(L, 3, 1);

    static const luaL_Reg s_meta[] = {
        { "__index",    _infra_range_index },
        { "__pairs",    _infra_range_pairs },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(L, INFRA_RANGE_NAME) != 0)
    {
        luaL_setfuncs(L, s_meta, 0);
    }
    lua_setmetatable(L, -2);

    return 1;
}

const infra_lua_api_t infra_f_range = {
"range", _infra_range, 0,
"Create a sequence of numbers.",

"[SYNOPSIS]\n"
"range range(start, stop, step)\n"
"\n"
"[DESCRIPTION]\n"
"The range() function returns a sequence of numbers, starting from 0 by default,\n"
"and increments by 1 (by default), and stops before a specified number.\n"
"\n"
"Parameter Values:\n"
"+ `start`: An integer number specifying at which position to start.\n"
"+ `stop`: An integer number specifying at which position to stop (not included).\n"
"+ `step`: Optional. An integer number specifying the incrementation. Default\n"
"  is 1.\n"
};
