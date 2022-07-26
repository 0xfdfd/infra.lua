#include "lua_int64.h"
#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define INT64_FLOAT_OPERAND_ERROR(L, v1, op, v2)   _infra_float_operands_error(L, #op)
#define INT64_PUSH_INT64(L, v1, op, v2)     infra_push_int64(L, v1 op v2)
#define INT64_PUSH_UINT64(L, v1, op, v2)    infra_push_uint64(L, v1 op v2)
#define INT64_PUSH_DOUBLE(L, v1, op, v2)    _infra_push_double(L, v1 op v2)
#define INT64_PUSH_BOOLEAN(L, v1, op, v2)   _infra_push_boolean(L, v1 op v2)

#define INT64_MATH_OP_TEMPLATE(L, idx1, idx2, OP, F_INT64, F_UINT64, F_FLOAT)   \
    do {\
        int64_val_t v1, v2;\
        _int64_get_and_upscale(L, idx1, idx2, &v1, &v2);\
        \
        switch (v1.type)\
        {\
        case INFRA_INT64:\
            return F_INT64(L, v1.u.as_d64, OP, v2.u.as_d64);\
        \
        case INFRA_UINT64:\
            return F_UINT64(L, v1.u.as_u64, OP, v2.u.as_u64);\
        \
        default:\
            return F_FLOAT(L, v1.u.as_f64, OP, v2.u.as_f64);\
        }\
    } while (0)

#define INT64_MATH_OP_FOR_INT(L, idx1, idx2, OP)    \
    INT64_MATH_OP_TEMPLATE(L, idx1, idx2, OP, INT64_PUSH_INT64, INT64_PUSH_UINT64, INT64_FLOAT_OPERAND_ERROR)

#define INT64_MATH_OP(L, idx1, idx2, OP)    \
    INT64_MATH_OP_TEMPLATE(L, idx1, idx2, OP, INT64_PUSH_INT64, INT64_PUSH_UINT64, INT64_PUSH_DOUBLE)

#define INT64_MATH_CMP(L, idx1, idx2, OP)   \
    INT64_MATH_OP_TEMPLATE(L, idx1, idx2, OP, INT64_PUSH_BOOLEAN, INT64_PUSH_BOOLEAN, INT64_PUSH_BOOLEAN)

#define INFRA_UINT64_OP(L, op) \
    uint64_t val1 = _to_uint64(L, 1);\
    uint64_t val2 = _to_uint64(L, 2);\
    return infra_push_uint64(L, val1 op val2)

#define INFRA_INT64_OP(L, op) \
    int64_t v1 = _to_int64(L, 1);\
    int64_t v2 = _to_int64(L, 2);\
    return infra_push_int64(L, v1 op v2)

static int _infra_push_double(lua_State* L, double val)
{
    lua_pushnumber(L, val);
    return 1;
}

static int _infra_push_boolean(lua_State* L, int val)
{
    lua_pushboolean(L, val);
    return 1;
}

static int _infra_float_operands_error(lua_State* L, const char* op)
{
    return luaL_error(L, "invalid operands to binary %s", op);
}

static uint64_t _to_uint64(lua_State* L, int idx)
{
    uint64_t ret;
    int val_type = infra_type(L, idx);

    /* Native number */
    if (val_type == LUA_TNUMBER)
    {
        if (lua_isinteger(L, idx))
        {
            return lua_tointeger(L, idx);
        }
        return lua_tonumber(L, idx);
    }

    if (val_type == INFRA_UINT64)
    {
        infra_uint64_t* u64_obj = lua_touserdata(L, idx);
        ret = u64_obj->value;
    }
    else if (val_type == INFRA_INT64)
    {
        infra_int64_t* d64_obj = lua_touserdata(L, idx);
        ret = d64_obj->value;
    }
    else
    {
        goto typeerror;
    }

    return ret;

typeerror:
    return infra_typeerror(L, idx, infra_type_name(INFRA_UINT64));
}

static int64_t _to_int64(lua_State* L, int idx)
{
    int64_t ret;
    int val_type = infra_type(L, idx);

    if (val_type == LUA_TNUMBER)
    {
        if (lua_isinteger(L, idx))
        {
            return lua_tointeger(L, idx);
        }
        return lua_tonumber(L, idx);
    }

    if (val_type == INFRA_UINT64)
    {
        infra_uint64_t* u64_obj = lua_touserdata(L, idx);
        ret = u64_obj->value;
    }
    else if (val_type == INFRA_INT64)
    {
        infra_int64_t* d64_obj = lua_touserdata(L, idx);
        ret = d64_obj->value;
    }
    else
    {
        goto typeerror;
    }

    return ret;

typeerror:
    return infra_typeerror(L, idx, infra_type_name(INFRA_INT64));
}

typedef struct int64_val
{
    /**
     * + INFRA_INT64:   int64_t
     * + INFRA_UINT64:  uint64_t
     * + LUA_TNUMBER:   double
     */
    int             type;
    union
    {
        int64_t     as_d64;
        uint64_t    as_u64;
        double      as_f64;
    } u;
} int64_val_t;

static void _int64_get_value(int64_val_t* v, lua_State* L, int idx)
{
    int val_type = infra_type(L, idx);
    if (val_type == LUA_TNUMBER)
    {
        if (lua_isinteger(L, idx))
        {
            v->type = INFRA_INT64;
            v->u.as_d64 = lua_tointeger(L, idx);
        }
        else
        {
            v->type = LUA_TNUMBER;
            v->u.as_f64 = lua_tonumber(L, idx);
        }
        return;
    }

    if (val_type == INFRA_INT64)
    {
        infra_int64_t* d64_obj = lua_touserdata(L, idx);
        v->type = INFRA_INT64;
        v->u.as_d64 = d64_obj->value;
        return;
    }

    if (val_type == INFRA_UINT64)
    {
        infra_uint64_t* u64_obj = lua_touserdata(L, idx);
        v->type = INFRA_UINT64;
        v->u.as_u64 = u64_obj->value;
        return;
    }

    infra_typeerror(L, idx, infra_type_name(INFRA_INT64));
}

static void _int64_upscale(lua_State* L, int64_val_t* v, int dst_type)
{
    if (v->type == dst_type)
    {
        return;
    }

    if (dst_type == INFRA_UINT64 && v->type == INFRA_INT64)
    {
        v->u.as_u64 = (uint64_t)v->u.as_d64;
    }
    else if (dst_type == LUA_TNUMBER && v->type == INFRA_INT64)
    {
        v->u.as_f64 = (double)v->u.as_d64;
    }
    else if (dst_type == LUA_TNUMBER && v->type == INFRA_UINT64)
    {
        v->u.as_f64 = (double)v->u.as_u64;
    }
    else
    {
        luaL_error(L, "can not upscale to lower type: src(%d) dst(%d)",
            v->type, dst_type);
        return;
    }

    v->type = dst_type;
}

static void _int64_get_and_upscale(lua_State* L, int idx1, int idx2, int64_val_t* v1, int64_val_t* v2)
{
    _int64_get_value(v1, L, idx1);
    _int64_get_value(v2, L, idx2);

    int max_type = v1->type > v2->type ? v1->type : v2->type;
    _int64_upscale(L, v1, max_type);
    _int64_upscale(L, v2, max_type);
}

static int _int64_add_compat(lua_State* L)
{
    INT64_MATH_OP(L, 1, 2, +);
}

static int _int64_sub_compat(lua_State* L)
{
    INT64_MATH_OP(L, 1, 2, -);
}

static int _int64_mul_compat(lua_State* L)
{
    INT64_MATH_OP(L, 1, 2, *);
}

static int _int64_div_compat(lua_State* L)
{
    INT64_MATH_OP(L, 1, 2, /);
}

static int _int64_mod_compat(lua_State* L)
{
    INT64_MATH_OP_FOR_INT(L, 1, 2, %);
}

static int _int64_pow_compat(lua_State* L)
{
    int64_val_t v1, v2;

    _int64_get_value(&v1, L, 1);
    _int64_get_value(&v2, L, 2);

    _int64_upscale(L, &v1, LUA_TNUMBER);
    _int64_upscale(L, &v2, LUA_TNUMBER);

    lua_pushnumber(L, pow(v1.u.as_f64, v2.u.as_f64));
    return 1;
}

static int _uint64_unm(lua_State* L)
{
    uint64_t val = _to_uint64(L, 1);
    return infra_push_uint64(L, -val);
}

static int _uint64_bor(lua_State* L)
{
    INFRA_UINT64_OP(L, |);
}

static int _uint64_shl(lua_State* L)
{
    INFRA_UINT64_OP(L, <<);
}

static int _uint64_shr(lua_State* L)
{
    INFRA_UINT64_OP(L, >>);
}

static int _int64_idiv_compat(lua_State* L)
{
    int64_val_t v1, v2;

    _int64_get_value(&v1, L, 1);
    _int64_get_value(&v2, L, 2);

    _int64_upscale(L, &v1, LUA_TNUMBER);
    _int64_upscale(L, &v2, LUA_TNUMBER);

    lua_pushnumber(L, v1.u.as_f64 / v2.u.as_f64);
    return 1;
}

static int _uint64_band(lua_State* L)
{
    INFRA_UINT64_OP(L, &);
}

static int _uint64_bxor(lua_State* L)
{
    INFRA_UINT64_OP(L, ^);
}

static int _uint64_bnot(lua_State* L)
{
    uint64_t val = _to_uint64(L, 1);
    return infra_push_uint64(L, ~val);
}

static int _int64_eq_compat(lua_State* L)
{
    INT64_MATH_CMP(L, 1, 2, ==);
}

static int _int64_lt_compat(lua_State* L)
{
    INT64_MATH_CMP(L, 1, 2, <);
}

static int _int64_le_compat(lua_State* L)
{
    INT64_MATH_CMP(L, 1, 2, <=);
}

static int _uint64_tostring(lua_State* L)
{
    char buf[21];
    uint64_t val = _to_uint64(L, 1);
    snprintf(buf, sizeof(buf), "%" PRIu64, val);
    lua_pushstring(L, buf);
    return 1;
}

static int _uint64_gc(lua_State* L)
{
    infra_uint64_t* u64_obj = lua_touserdata(L, 1);
    infra_exit_object(&u64_obj->object);
    return 0;
}

static void _uint64_set_meta(lua_State* L)
{
    static const luaL_Reg s_uint64_meta[] = {
        { "__add",  _int64_add_compat },
        { "__sub",  _int64_sub_compat },
        { "__mul",  _int64_mul_compat },
        { "__div",  _int64_div_compat },
        { "__mod",  _int64_mod_compat },
        { "__pow",  _int64_pow_compat },
        { "__unm",  _uint64_unm },
        { "__bor",  _uint64_bor },
        { "__shl",  _uint64_shl },
        { "__shr",  _uint64_shr },
        { "__idiv", _int64_idiv_compat },
        { "__band", _uint64_band },
        { "__bxor", _uint64_bxor },
        { "__bnot",     _uint64_bnot },
        { "__eq",       _int64_eq_compat },
        { "__lt",       _int64_lt_compat },
        { "__le",       _int64_le_compat },
        { "__tostring", _uint64_tostring },
        { "__gc",       _uint64_gc },
        { NULL,         NULL },
    };

    if (luaL_newmetatable(L, infra_type_name(INFRA_UINT64)) != 0)
    {
        luaL_setfuncs(L, s_uint64_meta, 0);
    }
    lua_setmetatable(L, -2);
}

static int _int64_bor(lua_State* L)
{
    INFRA_INT64_OP(L, |);
}

static int _int64_unm(lua_State* L)
{
    int64_t val = _to_int64(L, 1);
    return infra_push_int64(L, -val);
}

static int _int64_shl_compat(lua_State* L)
{
    INT64_MATH_OP_FOR_INT(L, 1, 2, <<);
}

static int _int64_shr(lua_State* L)
{
    INFRA_INT64_OP(L, >>);
}

static int _int64_band(lua_State* L)
{
    INFRA_INT64_OP(L, &);
}

static int _int64_bxor(lua_State* L)
{
    INFRA_INT64_OP(L, ^);
}

static int _int64_bnot(lua_State* L)
{
    int64_t val = _to_int64(L, 1);
    return infra_push_int64(L, ~val);
}

static int _int64_tostring(lua_State* L)
{
    char buf[32];
    int64_t val = _to_int64(L, 1);
    snprintf(buf, sizeof(buf), "%" PRId64, val);
    lua_pushstring(L, buf);
    return 1;
}

static int _int64_gc(lua_State* L)
{
    infra_int64_t* d64_obj = lua_touserdata(L, 1);
    infra_exit_object(&d64_obj->object);
    return 0;
}

static void _int64_set_meta(lua_State* L)
{
    static const luaL_Reg s_int64_meta[] = {
        { "__add",      _int64_add_compat },
        { "__sub",      _int64_sub_compat },
        { "__mul",      _int64_mul_compat },
        { "__div",      _int64_div_compat },
        { "__mod",      _int64_mod_compat },
        { "__pow",      _int64_pow_compat },
        { "__unm",      _int64_unm },
        { "__bor",      _int64_bor },
        { "__shl",      _int64_shl_compat },
        { "__shr",      _int64_shr },
        { "__idiv",     _int64_idiv_compat },
        { "__band",     _int64_band },
        { "__bxor",     _int64_bxor },
        { "__bnot",     _int64_bnot },
        { "__eq",       _int64_eq_compat },
        { "__lt",       _int64_lt_compat },
        { "__le",       _int64_le_compat },
        { "__tostring", _int64_tostring },
        { "__gc",       _int64_gc },
        { NULL,         NULL },
    };

    if (luaL_newmetatable(L, infra_type_name(INFRA_INT64)) != 0)
    {
        luaL_setfuncs(L, s_int64_meta, 0);
    }
    lua_setmetatable(L, -2);
}

int infra_push_uint64(lua_State* L, uint64_t val)
{
    infra_uint64_t* u64_obj = lua_newuserdata(L, sizeof(infra_uint64_t));
    infra_init_object(&u64_obj->object, L, INFRA_UINT64);
    u64_obj->value = val;
    _uint64_set_meta(L);
    return 1;
}

int infra_push_int64(lua_State* L, int64_t val)
{
    infra_int64_t* d64_obj = lua_newuserdata(L, sizeof(infra_int64_t));
    infra_init_object(&d64_obj->object, L, INFRA_INT64);
    d64_obj->value = val;
    _int64_set_meta(L);
    return 1;
}

int infra_uint64(lua_State* L)
{
    uint64_t v1 = luaL_checkinteger(L, 1);
    uint64_t v2 = luaL_checkinteger(L, 2);
    return infra_push_uint64(L, (v1 << 32) + v2);
}

int infra_int64(lua_State* L)
{
    int64_t v1 = luaL_checkinteger(L, 1);
    int64_t v2 = luaL_checkinteger(L, 2);
    return infra_push_int64(L, (v1 << 32) + v2);
}

int64_t infra_get_int64(lua_State* L, int idx)
{
    infra_int64_t* d64_obj = lua_touserdata(L, idx);
    return d64_obj->value;
}

uint64_t infra_get_uint64(lua_State* L, int idx)
{
    infra_uint64_t* u64_obj = lua_touserdata(L, idx);
    return u64_obj->value;
}
