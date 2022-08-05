#include "lua_json.h"
#include "lua_constant.h"
#include "lua_object.h"
#include "lua_int64.h"
#include "lua_list.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <cjson/cJSON.h>

typedef struct dump_helper
{
    lua_State*      L;          /**< Lua VM */
    luaL_Buffer     buf;        /**< Result buffer */
    char            tmp[64];    /**< Temporary buffer*/
} dump_helper_t;

typedef struct infra_json_s
{
    int useless;
} infra_json_t;

static void _t2j_dump_value_to_buf(dump_helper_t* helper, int* cnt, int idx);

static void _t2j_dump_string_as_json(dump_helper_t* helper, int idx)
{
    size_t len;
    size_t i;
    const char* str = lua_tolstring(helper->L, idx, &len);
    assert(str != NULL);

    luaL_addchar(&helper->buf, '"');
    for (i = 0; i < len; i++)
    {
        switch (str[i])
        {
        case '\b':
            luaL_addlstring(&helper->buf, "\\b", 2);
            break;
        case '\f':
            luaL_addlstring(&helper->buf, "\\f", 2);
            break;
        case '\n':
            luaL_addlstring(&helper->buf, "\\n", 2);
             break;
        case '\r':
            luaL_addlstring(&helper->buf, "\\r", 2);
            break;
        case '\t':
            luaL_addlstring(&helper->buf, "\\t", 2);
            break;
        case '\"':
            luaL_addlstring(&helper->buf, "\\\"", 2);
            break;
        case '\\':
            luaL_addlstring(&helper->buf, "\\\\", 2);
            break;
        default:
            luaL_addchar(&helper->buf, str[i]);
            break;
        }
    }

    luaL_addchar(&helper->buf, '"');
}

static int _table_is_array(lua_State* L, int idx)
{
    lua_pushnil(L);
    while (lua_next(L, idx) != 0)
    {
        if (!lua_isinteger(L, -2))
        {
            lua_pop(L, 2);
            return 0;
        }

        lua_pop(L, 1);
    }

    return 1;
}

static void _t2j_dump_table_as_json_template(dump_helper_t* helper, int* cnt,
    int idx, int is_array, int(*next)(lua_State*, int))
{
    int new_cnt = 0;
    int top = lua_gettop(helper->L);

    luaL_addstring(&helper->buf, is_array ? "[" : "{");
    cnt = &new_cnt;

    lua_pushnil(helper->L);
    while (next(helper->L, idx) != 0)
    {
        if (*cnt > 0)
        {
            luaL_addchar(&helper->buf, ',');
        }

        if (!is_array)
        {
            _t2j_dump_value_to_buf(helper, cnt, top + 1);
            luaL_addlstring(&helper->buf, ":", 1);
        }

        _t2j_dump_value_to_buf(helper, cnt, top + 2);

        lua_pop(helper->L, 1);
    }

    luaL_addstring(&helper->buf, is_array ? "]": "}");
}

static void _t2j_dump_table_as_json(dump_helper_t* helper, int* cnt, int idx)
{
    int is_array = _table_is_array(helper->L, idx);
    _t2j_dump_table_as_json_template(helper, cnt, idx, is_array, lua_next);
}

static void _t2j_dump_array_as_json(dump_helper_t* helper, int* cnt, int idx)
{
    _t2j_dump_table_as_json_template(helper, cnt, idx, 1, infra_list_next);
}

/* securely comparison of floating-point variables */
static int compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

static void _t2j_dump_number(dump_helper_t* helper, double val)
{
    size_t len;
    int d_val = (int)val;

    /* nan and inf treat as null */
    if (isnan(val) || isinf(val))
    {
        luaL_addlstring(&helper->buf, "null", 4);
        return;
    }

    /* We are able to hold value by int */
    if ((double)d_val == val)
    {
        len = snprintf(helper->tmp, sizeof(helper->tmp), "%d", d_val);
        luaL_addlstring(&helper->buf, helper->tmp, len);
        return;
    }

    /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
    len = snprintf(helper->tmp, sizeof(helper->tmp), "%1.15g", val);

    double test;
    /* Check whether the original double can be recovered */
    if ((sscanf(helper->tmp, "%lg", &test) != 1) || !compare_double(test, val))
    {
        /* If not, print with 17 decimal places of precision */
        len = snprintf(helper->tmp, sizeof(helper->tmp), "%1.17g", val);
    }

    luaL_addlstring(&helper->buf, helper->tmp, len);
}

static void _t2j_dump_value_to_buf(dump_helper_t* helper, int* cnt, int idx)
{
    int val_type = infra_type(helper->L, idx);

    *cnt += 1;

    switch (val_type)
    {
    case LUA_TNIL:
        luaL_addlstring(&helper->buf, "null", 4);
        break;

    case LUA_TNUMBER:
        _t2j_dump_number(helper, lua_tonumber(helper->L, idx));
        break;

    case LUA_TBOOLEAN:
        if (lua_toboolean(helper->L, idx))
        {
            luaL_addlstring(&helper->buf, "true", 4);
        }
        else
        {
            luaL_addlstring(&helper->buf, "false", 5);
        }
        break;

    case LUA_TSTRING:
        _t2j_dump_string_as_json(helper, idx);
        break;

    case LUA_TTABLE:
        _t2j_dump_table_as_json(helper, cnt, idx);
        break;

    case LUA_TLIGHTUSERDATA:
        if (lua_touserdata(helper->L, idx) == NULL)
        {
            luaL_addlstring(&helper->buf, "null", 4);
        }
        break;

    case INFRA_INT64:
        _t2j_dump_number(helper, (double)infra_get_int64(helper->L, idx));
        break;

    case INFRA_UINT64:
        _t2j_dump_number(helper, (double)infra_get_uint64(helper->L, idx));
        break;

    case INFRA_LIST:
        _t2j_dump_array_as_json(helper, cnt, idx);
        break;

    default:
        break;
    }
}

static int _json_gc(lua_State* L)
{
    infra_json_t* json = luaL_checkudata(L, 1, infra_type_name(INFRA_JSON));
    (void)json;
    return 0;
}

static int _json_encode(lua_State* L)
{
    if (!lua_istable(L, 2))
    {
        return infra_typeerror(L, 1, "table");
    }

    dump_helper_t helper;
    helper.L = L;
    luaL_buffinit(L, &helper.buf);

    int cnt = 0;
    _t2j_dump_value_to_buf(&helper, &cnt, 2);

    luaL_pushresult(&helper.buf);
    return 1;
}

/**
 * @brief Parser \p json as lua native value and push onto stack \p L.
 * @param[in] L     Lua VM.
 * @param[in] json  Json object.
 * @return          Always 1.
 */
static int _json_object_to_lua_table(lua_State* L, cJSON* json)
{
    cJSON* obj = NULL;

    /* Array: add to table */
    if (cJSON_IsArray(json))
    {
        lua_newtable(L);
        size_t arr_idx = 1;
        cJSON_ArrayForEach(obj, json)
        {
            _json_object_to_lua_table(L, obj);
            lua_rawseti(L, -2, arr_idx++);
        }
        return 1;
    }

    /* Object: add to table */
    if (cJSON_IsObject(json))
    {
        lua_newtable(L);
        cJSON_ArrayForEach(obj, json)
        {
            lua_pushstring(L, obj->string);
            _json_object_to_lua_table(L, obj);
            lua_rawset(L, -3);
        }
        return 1;
    }

    if (cJSON_IsString(json))
    {
        lua_pushstring(L, json->valuestring);
        return 1;
    }

    if (cJSON_IsNumber(json))
    {
        if ((double)json->valueint == json->valuedouble)
        {
            lua_pushinteger(L, json->valueint);
        }
        else
        {
            lua_pushnumber(L, json->valuedouble);
        }
        return 1;
    }

    if (cJSON_IsNull(json))
    {
        lua_pushlightuserdata(L, NULL);
        return 1;
    }

    if (cJSON_IsTrue(json))
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    if (cJSON_IsFalse(json))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    return 0;
}

static int _json_decode(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 2, &len);

    cJSON* json = cJSON_ParseWithLength(str, len);
    if (json == NULL)
    {
        return 0;
    }

    /* Convert json to table */
    _json_object_to_lua_table(L, json);

    cJSON_Delete(json);

    return 1;
}

static void _json_set_meta(lua_State* L)
{
    static const luaL_Reg s_json_meta[] = {
        { "__gc",   _json_gc },
        { NULL,     NULL },
    };
    static const luaL_Reg s_json_method[] = {
        { "encode", _json_encode },
        { "decode", _json_decode },
        { NULL,     NULL },
    };
    if (luaL_newmetatable(L, infra_type_name(INFRA_JSON)) != 0)
    {
        luaL_setfuncs(L, s_json_meta, 0);

        /* metatable.__index = s_json_method */
        luaL_newlib(L, s_json_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
}

int infra_new_json(lua_State* L)
{
    infra_json_t* json = lua_newuserdata(L, sizeof(infra_json_t));
    json->useless = 0;
    _json_set_meta(L);
    return 1;
}
