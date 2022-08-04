#include "lua_list.h"
#include "lua_object.h"
#include "utils/list.h"

typedef struct infra_list
{
    infra_object_t      object; /**< Base object */
    ev_list_t           list;   /**< Internal list */
} infra_list_t;

typedef struct infra_list_node
{
    ev_list_node_t      node;
    int                 v_ref;  /**< Refer to value */
    int                 s_ref;  /**< Refer to self */
    infra_list_t*       belong;
} infra_list_node_t;

/**
 * @brief Push value at \p v into list at \p idx with function \p cb. The
 *   iterator is pushed onto stack.
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @param[in] v     Value index.
 * @param[in] cb    List operation.
 * @return          Always 1.
 */
static int _list_push_template(lua_State* L, int idx, int v,
    void(*cb)(ev_list_t*, ev_list_node_t*))
{
    int cur_idx = lua_gettop(L);

    infra_list_t* p_list = luaL_checkudata(L, idx, infra_type_name(INFRA_LIST));

    /* Create node */
    infra_list_node_t* node = lua_newuserdata(L, sizeof(infra_list_node_t));
    node->belong = p_list;

    /* Refer to self */
    lua_pushvalue(L, cur_idx + 1);
    node->s_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Refer to value */
    lua_pushvalue(L, v);
    node->v_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Push to list */
    cb(&p_list->list, &node->node);

    return 1;
}

static int _list_push_front(lua_State* L)
{
    return _list_push_template(L, 1, 2, ev_list_push_front);
}

/**
 * @brief Push back value at \p v into list at \p idx. The iterator is pushed
 *   onto stack.
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @param[in] v     Value index.
 * @return          Always 1.
 */
static int _list_push_back_detail(lua_State* L, int idx, int v)
{
    return _list_push_template(L, idx, v, ev_list_push_back);
}

static int _list_push_back(lua_State* L)
{
    return _list_push_back_detail(L, 1, 2);
}

static void _list_unref_node(lua_State* L, infra_list_t* p_list,
    infra_list_node_t* node)
{
    if (node->belong != NULL)
    {
        ev_list_erase(&p_list->list, &node->node);
        node->belong = NULL;
    }
    if (node->v_ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, node->v_ref);
        node->v_ref  = LUA_NOREF;
    }
    if (node->s_ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, node->s_ref);
        node->s_ref = LUA_NOREF;
    }
}

/**
 * @brief Pop a element from list at \p idx onto top of stack with list
 *   operation \p cb.
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @param[in] cb    List operation.
 * @return          0 if empty, 1 if success.
 */
static int _list_pop_template(lua_State* L, int idx,
    ev_list_node_t*(*cb)(ev_list_t*))
{
    infra_list_t* p_list = luaL_checkudata(L, idx, infra_type_name(INFRA_LIST));
    ev_list_node_t* it = cb(&p_list->list);
    if (it == NULL)
    {
        return 0;
    }

    infra_list_node_t* node = container_of(it, infra_list_node_t, node);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->v_ref);

    node->belong = NULL;
    _list_unref_node(L, p_list, node);

    return 1;
}

static int _list_pop_front(lua_State* L)
{
    return _list_pop_template(L, 1, ev_list_pop_front);
}

static int _list_pop_back(lua_State* L)
{
    return _list_pop_template(L, 1, ev_list_pop_back);
}

static int _list_size(lua_State* L)
{
    infra_list_t* p_list = luaL_checkudata(L, 1, infra_type_name(INFRA_LIST));
    lua_pushinteger(L, ev_list_size(&p_list->list));
    return 1;
}

static int _list_gc(lua_State* L)
{
    infra_list_t* p_list = lua_touserdata(L, 1);

    /* Release every node */
    ev_list_node_t* it;
    while ((it = ev_list_pop_front(&p_list->list)) != NULL)
    {
        infra_list_node_t* node = container_of(it, infra_list_node_t, node);

        node->belong = NULL;
        _list_unref_node(L, p_list, node);
    }

    infra_exit_object(&p_list->object);
    return 0;
}

static int _list_pairs_next(lua_State* L)
{
    ev_list_node_t* it;
    infra_list_node_t* node;
    infra_list_t* p_list = luaL_checkudata(L, 1, infra_type_name(INFRA_LIST));

    /* First key */
    if (lua_type(L, 2) == LUA_TNIL)
    {
        it = ev_list_begin(&p_list->list);

        /* List is empty */
        if (it == NULL)
        {
            goto no_more_index;
        }

        goto push_index_value;
    }

    node = lua_touserdata(L, 2);
    it = ev_list_next(&node->node);
    if (it == NULL)
    {
        goto no_more_index;
    }
    goto push_index_value;

push_index_value:
    node = container_of(it, infra_list_node_t, node);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->s_ref);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->v_ref);
    return 2;

no_more_index:
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

static int _list_pairs(lua_State* L)
{
    lua_pushcfunction(L, _list_pairs_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int _list_erase(lua_State* L)
{
    infra_list_t* p_list = luaL_checkudata(L, 1, infra_type_name(INFRA_LIST));
    infra_list_node_t* node = lua_touserdata(L, 2);

    _list_unref_node(L, p_list, node);

    return 0;
}

static int _list_begin(lua_State* L)
{
    infra_list_t* p_list = luaL_checkudata(L, 1, infra_type_name(INFRA_LIST));
    ev_list_node_t* it = ev_list_begin(&p_list->list);
    if (it == NULL)
    {
        return 0;
    }

    infra_list_node_t* node = container_of(it, infra_list_node_t, node);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->s_ref);
    return 1;
}

static int _list_next(lua_State* L)
{
    infra_list_node_t* node = lua_touserdata(L, 2);
    if (node == NULL || node->belong == NULL)
    {
        return 0;
    }

    ev_list_node_t* it = ev_list_next(&node->node);
    if (it == NULL)
    {
        return 0;
    }

    node = container_of(it, infra_list_node_t, node);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->s_ref);
    return 1;
}

int infra_list_next(lua_State* L, int idx)
{
    ev_list_node_t* it;
    infra_list_t* p_list = lua_touserdata(L, idx);
    infra_list_node_t* node = lua_touserdata(L, -1);

    /* The first key-value pair */
    if (node == NULL)
    {
        /* Pop previous key */
        lua_pop(L, 1);

        /* Get first list element */
        if ((it = ev_list_begin(&p_list->list)) == NULL)
        {
            return 0;
        }

        /* Get first list node iterator */
        node = container_of(it, infra_list_node_t, node);

        /* Push iterator */
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->s_ref);
        /* Push value */
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->v_ref);

        return 1;
    }

    /* Get next node */
    it = ev_list_next(&node->node);

    /* Pop previous key */
    lua_pop(L, 1);

    /* No next element */
    if (it == NULL)
    {
        return 0;
    }

    /* Get next iterator */
    node = container_of(it, infra_list_node_t, node);

    /* Push iterator */
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->s_ref);
    /* Push value */
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->v_ref);

    return 1;
}

/**
 * @brief Set metatable for top of stack value.
 * @param[in] L     Lua VM.
 */
static void _list_set_meta(lua_State* L)
{
    static const luaL_Reg s_list_meta[] = {
        { "__gc",       _list_gc },
        { "__pairs",    _list_pairs },
        { NULL,         NULL },
    };
    static const luaL_Reg s_list_method[] = {
        { "push_front", _list_push_front },
        { "push_back",  _list_push_back },
        { "pop_front",  _list_pop_front },
        { "pop_back",   _list_pop_back },
        { "begin",      _list_begin },
        { "next",       _list_next },
        { "erase",      _list_erase },
        { "size",       _list_size },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(L, infra_type_name(INFRA_LIST)) != 0)
    {
        luaL_setfuncs(L, s_list_meta, 0);

        /* metatable.__index = s_list_method */
        luaL_newlib(L, s_list_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
}

/**
 * @brief Copy table at \p t into list at \p idx
 * @param[in] L     Lua VM.
 * @param[in] idx   List index.
 * @param[in] t     Table index.
 * @return          Always 0.
 */
static int _list_copy_from_table(lua_State* L, int idx, int t)
{
    /* Record stack top */
    int stack_top = lua_gettop(L);

    /* Sort key, the stack should remain balance. */
    lua_pushnil(L);
    while (lua_next(L, t) != 0)
    {
        if (lua_type(L, -2) != LUA_TNUMBER)
        {
            return luaL_error(L, "table contains non-integer key.");
        }

        /* Push into list and push iterator onto stack */
        _list_push_back_detail(L, idx, stack_top + 2);

        /* Cleanup value from #lua_next() and #_list_push_back_detail() */
        lua_pop(L, 2);
    }

    return 0;
}

int infra_new_list(lua_State* L)
{
    /* Check if we have any argument */
    int arg_type = lua_type(L, 1);

    /* Push list onto stack */
    infra_list_t* obj = lua_newuserdata(L, sizeof(infra_list_t));

    /* Initialize list */
    infra_init_object(&obj->object, L, INFRA_LIST);
    ev_list_init(&obj->list);
    _list_set_meta(L);

    if (arg_type == LUA_TTABLE)
    {
        _list_copy_from_table(L, 2, 1);
    }

    return 1;
}
