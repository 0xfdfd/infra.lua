#include "__init__.h"

static int _compare_metamethod(lua_State* L, const char* method, int idx1, int idx2, int* ret)
{
    int sp = lua_gettop(L);

    if (luaL_getmetafield(L, idx1, method) == 0)
    {
        return 0;
    }

    lua_pushvalue(L, idx1);
    lua_pushvalue(L, idx2);
    lua_call(L, 2, 1);

    *ret = lua_toboolean(L, -1);

    lua_settop(L, sp);
    return 1;
}

/**
 * @brief Compare with metamethod.
 */
static int _internal_compare_with_meta(lua_State* L, int idx1, int idx2, int* ret)
{
    int tmp;

    int ret_1_ne_2 = 0;
    if (_compare_metamethod(L, "__eq", idx1, idx2, &tmp))
    {
        if (tmp)
        {
            *ret = 0;
            return 1;
        }
        ret_1_ne_2 = 1;
    }
    if (_compare_metamethod(L, "__eq", idx2, idx1, &tmp))
    {
        if (tmp)
        {
            *ret = 0;
            return 1;
        }
        ret_1_ne_2 = 1;
    }

    int ret_1_ge_2 = 0;
    if (_compare_metamethod(L, "__lt", idx1, idx2, &tmp))
    {
        if (tmp)
        {
            *ret = -1;
            return 1;
        }

        /* Here idx1 >= idx2 */
        ret_1_ge_2 = 1;

        /* If they are not equal */
        if (ret_1_ne_2)
        {
            *ret = 1;
            return 1;
        }
    }

    int ret_1_le_2 = 0;
    if (_compare_metamethod(L, "__le", idx1, idx2, &tmp))
    {
        if (!tmp)
        {
            *ret = 1;
            return 1;
        }

        /* Here idx1 <= idx2 */
        ret_1_le_2 = 1;

        /* If they are not equal, idx1 < idx2. */
        if (ret_1_ne_2)
        {
            *ret = -1;
            return 1;
        }

        /* If idx1 >= idx2, then they are equal. */
        if (ret_1_ge_2)
        {
            *ret = 0;
            return 1;
        }
    }

    if (_compare_metamethod(L, "__lt", idx2, idx1, &tmp))
    {
        if (tmp)
        {/* idx1 > idx2 */
            *ret = 1;
            return 1;
        }
        /* idx1 <= idx2 */
        ret_1_le_2 = 1;

        if (ret_1_ne_2)
        {
            *ret = -1;
            return 1;
        }
    }

    if (_compare_metamethod(L, "__le", idx2, idx1, &tmp))
    {
        if (!tmp)
        {
            *ret = -1;
            return 1;
        }
        /* idx1 >= idx2 */
        ret_1_ge_2 = 1;

        if (ret_1_ne_2)
        {
            *ret = 1;
            return 1;
        }

        if (ret_1_le_2)
        {
            *ret = 0;
            return 1;
        }
    }


    /* Compare failure */
    return 0;
}

static int _internal_compare_as_number(lua_State* L, int idx1, int idx2)
{
    lua_Number n1 = lua_tonumber(L, idx1);
    lua_Number n2 = lua_tonumber(L, idx2);
    if (n1 < n2)
    {
        return -1;
    }
    else if (n1 > n2)
    {
        return 1;
    }
    
    return 0;
}

static int _internal_compare_as_boolean(lua_State* L, int idx1, int idx2)
{
    int n1 = lua_toboolean(L, idx1);
    int n2 = lua_toboolean(L, idx2);
    if (n1 < n2)
    {
        return -1;
    }
    else if (n1 > n2)
    {
        return 1;
    }
    
    return 0;
}

static int _internal_compare_as_string(lua_State* L, int idx1, int idx2)
{
    size_t dat1_sz = 0;
    const char* dat1 = lua_tolstring(L, idx1, &dat1_sz);

    size_t dat2_sz = 0;
    const char* dat2 = lua_tolstring(L, idx2, &dat2_sz);

    size_t pos = 0;
    for (; pos < dat1_sz && pos < dat2_sz; pos++)
    {
        if (dat1[pos] < dat2[pos])
        {
            return -1;
        }
        else if (dat1[pos] > dat2[pos])
        {
            return 1;
        }
    }
    if (dat1_sz < dat2_sz)
    {
        return -1;
    }
    if (dat1_sz > dat2_sz)
    {
        return 1;
    }
    return 0;
}

static int _internal_compare_as_pointer(lua_State* L, int idx1, int idx2)
{
    const unsigned char* p1 = lua_topointer(L, idx1);
    const unsigned char* p2 = lua_topointer(L, idx2);
    if (p1 < p2)
    {
        return -1;
    }
    else if (p1 > p2)
    {
        return 1;
    }
    return 0;
}

static int _internal_compare_same_type(lua_State* L, int idx1, int idx2, int v_type, int* ret)
{
    switch (v_type)
    {
    case LUA_TNIL:
        *ret = 0;
        return 1;

    case LUA_TNUMBER:
        *ret = _internal_compare_as_number(L, idx1, idx2);
        return 1;

    case LUA_TBOOLEAN:
        *ret = _internal_compare_as_boolean(L, idx1, idx2);
        return 1;

    case LUA_TSTRING:
        *ret = _internal_compare_as_string(L, idx1, idx2);
        return 1;

    case LUA_TUSERDATA:
    case LUA_TTABLE:
    case LUA_TTHREAD:
    case LUA_TFUNCTION:
        *ret = _internal_compare_as_pointer(L, idx1, idx2);
        return 1;

    default:
        break;
    }

    return 0;
}

static int _internal_compare_without_meta(lua_State* L, int idx1, int idx2, int* ret)
{
    if (lua_rawequal(L, idx1, idx2))
    {
        *ret = 0;
        return 1;
    }

    int t1 = lua_type(L, idx1);
    int t2 = lua_type(L, idx2);
    if (t1 < t2)
    {
        *ret = -1;
        return 1;
    }
    else if (t1 > t2)
    {
        *ret = 1;
        return 1;
    }

    return _internal_compare_same_type(L, idx1, idx2, t1, ret);
}

static int _internal_compare(lua_State* L, int idx1, int idx2)
{
    int ret = 0;

    if (_internal_compare_without_meta(L, idx1, idx2, &ret))
    {
        return ret;
    }

    if (_internal_compare_with_meta(L, idx1, idx2, &ret))
    {
        return ret;
    }

    return luaL_error(L, "Cannot do compare.");
}

static int _compare(lua_State* L)
{
    int ret = _internal_compare(L, 1, 2);
    lua_pushinteger(L, ret);
    return 1;
}

const infra_lua_api_t infra_f_compare = {
"compare", _compare, 0,
"Compare two values.",

"[SYNOPSIS]\n"
"number compare(object v1, object v2)\n"
"\n"
"[DESCRIPTION]\n"
"Compare two Lua value, and returns an integer indicating the result of the\n"
"comparison, as follows:\n"
"    0: v1 and v2 are equal;\n"
"    1: v1 is greater than v2;\n"
"    -1: v1 is less than v2.\n"
};
