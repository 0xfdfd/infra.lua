#include "lua_json.h"
#include "lua_constant.h"
#include "lua_errno.h"
#include "lua_int64.h"
#include "lua_object.h"
#include "lua_list.h"
#include "utils/list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>

typedef struct dump_helper
{
    lua_State*      L;          /**< Lua VM */
    luaL_Buffer     buf;        /**< Result buffer */
    char            tmp[64];    /**< Temporary buffer*/
} dump_helper_t;

typedef struct json_parser_record
{
    ev_list_node_t          node;

    struct
    {
        /**
         * Where it start.
         * If it is a string, the start should contains quote (")
         * If it is a table, the start contains left brace ([)
         * If it is a array, the start contains left square bracket ([)
         */
        size_t              offset;

        /**
         * The stack index for this element.
         */
        int                 stack_idx;

        /**
         * (Array) The array index.
         */
        int                 array_idx;
    } data;

    unsigned                mask;
} json_parser_record_t;

typedef enum json_parser_record_mask
{
    JSON_TABLE   = 0x01 << 0x00,
    JSON_ARRAY   = 0x01 << 0x01,
    JSON_STRING  = 0x01 << 0x02,
    JSON_NUMBER  = 0x01 << 0x03,
} json_parser_record_mask_t;

typedef struct json_parser
{
    lua_State*              L;          /**< Lua VM */
    const char*             data;       /**< Json string */
    size_t                  size;       /**< Json string length */
    size_t                  offset;     /**< Where we are going to access */
    ev_list_t               r_stack;    /**< Parser stack */
} json_parser_t;

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

int infra_table_to_json(lua_State* L)
{
    if (!lua_istable(L, 1))
    {
        return infra_typeerror(L, 1, "table");
    }

    dump_helper_t helper;
    helper.L = L;
    luaL_buffinit(L, &helper.buf);

    int cnt = 0;
    _t2j_dump_value_to_buf(&helper, &cnt, 1);

    luaL_pushresult(&helper.buf);
    return 1;
}

static int _json_save_as(json_parser_t* parser, size_t offset, unsigned mask)
{
    json_parser_record_t* rec = calloc(1, sizeof(json_parser_record_t));
    CHECK_OOM(parser->L, rec);

    rec->data.offset = offset;
    rec->mask = mask;

    ev_list_push_back(&parser->r_stack, &rec->node);
    parser->offset = offset + 1;

    return INFRA_SUCCESS;
}

static size_t _json_bypass_whitespace(json_parser_t* parser, size_t offset)
{
    for (; offset < parser->size; offset++)
    {
        if (parser->data[offset] != ' ' && parser->data[offset] != '\t')
        {
            return offset;
        }
    }
    return parser->size;
}

static json_parser_record_t* _json_prev_node(json_parser_record_t* rec)
{
    ev_list_node_t* it = ev_list_prev(&rec->node);
    if (it == NULL)
    {
        return NULL;
    }
    return container_of(it, json_parser_record_t, node);
}

static int _json_finish_item(json_parser_t* parser, json_parser_record_t* rec, size_t offset)
{
    /* This object should be on top of stack */
    assert(lua_gettop(parser->L) == rec->data.stack_idx);

    /* Update global offset */
    parser->offset = offset + 1;

    /* Get parent node */
    json_parser_record_t* parent = _json_prev_node(rec);
    if (parent == NULL)
    {
        return INFRA_SUCCESS;
    }

    /* Parent is table / array */
    if (parent->mask & (JSON_TABLE | JSON_ARRAY))
    {
        if (parent->data.stack_idx == rec->data.stack_idx - 1)
        {/* This is the key */
            if (!(rec->mask & JSON_STRING))
            {/* Only string can be key */
                return INFRA_EINVAL;
            }
            return INFRA_SUCCESS;
        }
        else if(parent->data.stack_idx == rec->data.stack_idx - 2)
        {/* This is the value */
            lua_settable(parser->L, parent->data.stack_idx);
            return INFRA_SUCCESS;
        }
        else
        {
            return luaL_error(parser->L, "invalid stack index: parent(%d) current(%d)",
                parent->data.stack_idx, rec->data.stack_idx);
        }
    }

    return INFRA_EINVAL;
}

static int _json_finish_array(json_parser_t* parser, json_parser_record_t* rec, size_t offset)
{
    return _json_finish_item(parser, rec, offset);
}

static int _json_finish_table(json_parser_t* parser, json_parser_record_t* rec, size_t offset)
{
    return _json_finish_item(parser, rec, offset);
}

static int _json_finish_string(json_parser_t* parser, json_parser_record_t* rec, size_t offset, luaL_Buffer* buf)
{
    /* Push result */
    luaL_pushresult(buf);

    /* Record stack index */
    rec->data.stack_idx = lua_gettop(parser->L);

    /* Finish parser */
    return _json_finish_item(parser, rec, offset);
}

static void _json_try_push_array_key(json_parser_t* parser, json_parser_record_t* rec)
{
    if (rec->mask & JSON_ARRAY)
    {
        lua_pushinteger(parser->L, rec->data.array_idx++);
    }
}

static int _json_process_table(json_parser_t* parser, json_parser_record_t* rec,
    unsigned mask)
{
    if (rec->data.stack_idx == 0)
    {
        lua_newtable(parser->L);
        rec->data.stack_idx = lua_gettop(parser->L);
        rec->data.array_idx = 1;
    }

    size_t offset = parser->offset;

    for (;;)
    {
        /* Get next token */
        offset = _json_bypass_whitespace(parser, offset);
        if (offset >= parser->size)
        {/* At least ']' should occur */
            return INFRA_EINVAL;
        }

        /* Table */
        if (parser->data[offset] == '{')
        {
            _json_try_push_array_key(parser, rec);
            _json_save_as(parser, offset, JSON_TABLE);
            return INFRA_EAGAIN;
        }

        /* Array */
        if (parser->data[offset] == '[')
        {
            _json_try_push_array_key(parser, rec);
            _json_save_as(parser, offset, JSON_ARRAY);
            return INFRA_EAGAIN;
        }

        /* String */
        if (parser->data[offset] == '\"')
        {
            _json_try_push_array_key(parser, rec);
            _json_save_as(parser, offset, JSON_STRING);
            return INFRA_EAGAIN;
        }

        /* Number */
        if ((parser->data[offset] >= '0' && parser->data[offset] <= '9')
            || parser->data[offset] == '-')
        {
            _json_try_push_array_key(parser, rec);
            _json_save_as(parser, offset, JSON_NUMBER);
            return INFRA_EAGAIN;
        }

        /* Comma */
        if (parser->data[offset] == ',')
        {
            offset++;
            continue;
        }

        /* Semicolons */
        if ((mask & JSON_TABLE) && parser->data[offset] == ':')
        {
            offset++;
            continue;
        }

        /* The table is finish */
        if ((mask & JSON_TABLE) && parser->data[offset] == '}')
        {
            return _json_finish_table(parser, rec, offset);
        }

        /* The array is finish */
        if ((mask & JSON_ARRAY) && parser->data[offset] == ']')
        {
            return _json_finish_array(parser, rec, offset);
        }

        /* Other conditions */
        break;
    }

    /* Invalid json */
    return INFRA_EINVAL;
}

static int _json_process_string(json_parser_t* parser, json_parser_record_t* rec)
{
    luaL_Buffer buf;
    luaL_buffinit(parser->L, &buf);

    int have_escape = 0;
    size_t offset = rec->data.offset + 1;
    for (; offset < parser->size; offset++)
    {
        if (have_escape)
        {
            have_escape = 0;
            switch (parser->data[offset])
            {
            case 'b': luaL_addchar(&buf, '\b'); break;
            case 'f': luaL_addchar(&buf, '\f'); break;
            case 'n': luaL_addchar(&buf, '\n'); break;
            case 'r': luaL_addchar(&buf, '\r'); break;
            case 't': luaL_addchar(&buf, '\t'); break;
            case '"': luaL_addchar(&buf, '"');  break;
            default:                            break;
            }
            continue;
        }

        if (parser->data[offset] == '\\')
        {
            have_escape = 1;
            continue;
        }

        if (parser->data[offset] == '"')
        {
            return _json_finish_string(parser, rec, offset, &buf);
        }

        luaL_addchar(&buf, parser->data[offset]);
    }

    return INFRA_EINVAL;
}

static int _json_process_number(json_parser_t* parser, json_parser_record_t* rec)
{
    int have_dot = 0;
    int have_exponent = 0;
    const char* num_beg = parser->data + rec->data.offset;
    const char* num_end = NULL;

    size_t idx = rec->data.offset;
    for (; idx < parser->size; idx++)
    {
        if (parser->data[idx] >= '0' && parser->data[idx] <= '9')
        {
            continue;
        }
        if (parser->data[idx] == '.')
        {
            if (have_dot)
            {/* Only can have one dot */
                return INFRA_EINVAL;
            }
            have_dot = 1;
            continue;
        }
        if (parser->data[idx] == 'e' || parser->data[idx] == 'E')
        {
            if (have_exponent)
            {
                return INFRA_EINVAL;
            }
            have_exponent = 1;
            continue;
        }
        if (parser->data[idx] == '+' || parser->data[idx] == '-')
        {
            continue;
        }

        num_end = &parser->data[idx];
        break;
    }

    /* Reach end early */
    if (num_end == NULL)
    {
        return INFRA_EINVAL;
    }

    size_t malloc_size = num_end - num_beg + 1;
    char* tmp_buffer = malloc(malloc_size);
    CHECK_OOM(parser->L, tmp_buffer);

    memcpy(tmp_buffer, num_beg, malloc_size);
    tmp_buffer[malloc_size - 1] = '\0';

    if (have_dot)
    {
        double val;
        if (sscanf(tmp_buffer, "%lf", &val) != 1)
        {
            return INFRA_EINVAL;
        }
        lua_pushnumber(parser->L, val);
    }
    else
    {
        long val;
        if (sscanf(tmp_buffer, "%ld", &val) != 1)
        {
            return INFRA_EINVAL;
        }
        lua_pushinteger(parser->L, val);
    }

    free(tmp_buffer);
    return _json_finish_item(parser, rec, idx);
}

static int _json_process_record(json_parser_t* parser, json_parser_record_t* rec)
{
    if (rec->mask & JSON_TABLE)
    {
        return _json_process_table(parser, rec, JSON_TABLE);
    }

    if (rec->mask & JSON_ARRAY)
    {
        return _json_process_table(parser, rec, JSON_ARRAY);
    }

    if (rec->mask & JSON_STRING)
    {
        return _json_process_string(parser, rec);
    }

    if (rec->mask & JSON_NUMBER)
    {
        return _json_process_number(parser, rec);
    }

    return luaL_error(parser->L, "invalid mask:%u", rec->mask);
}

/**
 * @param L
 * @param data
 * @param size
 * @param offset
 * @return The number of bytes passed.
 */
static int _json_to_table(json_parser_t* parser)
{
    ev_list_node_t* it;
    int ret;

    while ((it = ev_list_end(&parser->r_stack)) != NULL)
    {
        json_parser_record_t* rec = container_of(it, json_parser_record_t, node);

        /* The last record is always on top of stack */
        ret = _json_process_record(parser, rec);

        if (ret == INFRA_EAGAIN)
        {
            continue;
        }

        if (ret == INFRA_SUCCESS)
        {
            ev_list_erase(&parser->r_stack, &rec->node);
            free(rec);
            continue;
        }

        return ret;
    }

    return INFRA_SUCCESS;
}

static int _init_json_parser(lua_State* L, json_parser_t* parser, const char* data, size_t size)
{
    parser->L = L;
    parser->data = data;
    parser->size = size;
    parser->offset = 0;
    ev_list_init(&parser->r_stack);

    size_t offset = 0;
    /* Skip white space */
    for (; offset < size; offset++)
    {
        if (data[offset] != ' ' && data[offset] != '\t')
        {
            break;
        }
    }

    /* Empty buffer */
    if (offset >= size)
    {
        return INFRA_EINVAL;
    }

    /* Check root node type */
    if (data[offset] == '{')
    {
        return _json_save_as(parser, offset, JSON_TABLE);
    }

    if (data[offset] == '[')
    {
        return _json_save_as(parser, offset, JSON_ARRAY);
    }

    /* Not a json object */
    return INFRA_EINVAL;
}

static void _json_cleanup_parser(json_parser_t* parser)
{
    ev_list_node_t* it;

    while ((it = ev_list_pop_front(&parser->r_stack)) != NULL)
    {
        json_parser_record_t* rec = container_of(it, json_parser_record_t, node);
        free(rec);
    }
}

int infra_json_to_table(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);

    int top1 = lua_gettop(L);

    json_parser_t parser;
    int ret = _init_json_parser(L, &parser, str, len);
    if (ret < 0)
    {
        lua_pushinteger(L, ret);
        return 1;
    }

    if ((ret = _json_to_table(&parser)) != 0)
    {
        int top2 = lua_gettop(L);
        if (top2 > top1)
        {
            lua_pop(L, top2 - top1);
        }

        lua_pushinteger(L, ret);
    }
    _json_cleanup_parser(&parser);

    return 1;
}
