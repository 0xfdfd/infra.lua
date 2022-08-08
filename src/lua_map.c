#include "lua_object.h"
#include "utils/map.h"
#include "utils/list.h"

typedef struct infra_map
{
    lua_State*      L;      /**< Lua VM */
    ev_map_t        table;  /**< Table */
    ev_list_t       queue;  /**< List */
} infra_map_t;

typedef struct infra_table_node
{
    ev_map_node_t   mnode;  /**< Table node */
    ev_list_node_t  qnode;  /**< Queue node */

    infra_map_t*    belong; /**< The map belong to */
    struct
    {
        int         s_ref;  /**< Reference to self */
        int         k_ref;  /**< Reference to key */
        int         v_ref;  /**< Reference to value */
    } data;
} infra_map_node_t;

static int _map_compare_node(infra_map_t* table, infra_map_node_t* n1,
    infra_map_node_t* n2)
{
    int ret;
    int v1_type = lua_rawgeti(table->L, LUA_REGISTRYINDEX, n1->data.k_ref);
    int v2_type = lua_rawgeti(table->L, LUA_REGISTRYINDEX, n2->data.k_ref);
    if (v1_type != v2_type)
    {
        ret = v1_type < v2_type ? -1: 1;
        goto finish;
    }

    if (lua_compare(table->L, -2, -1, LUA_OPLT))
    {
        ret = -1;
        goto finish;
    }

    if (lua_compare(table->L, -1, -2, LUA_OPLT))
    {
        ret = 1;
        goto finish;
    }

    ret = 0;

finish:
    lua_pop(table->L, 2);
    return ret;
}

static int _on_cmp_node(const ev_map_node_t* key1, const ev_map_node_t* key2,
    void* arg)
{
    infra_map_t* table = arg;
    infra_map_node_t* n1 = container_of(key1, infra_map_node_t, mnode);
    infra_map_node_t* n2 = container_of(key2, infra_map_node_t, mnode);
    return _map_compare_node(table, n1, n2);
}

static int _map_erase_node(infra_map_t* table, infra_map_node_t* node, int erase)
{
    if (erase)
    {
        node->belong = NULL;
        ev_map_erase(&table->table, &node->mnode);
        ev_list_erase(&table->queue, &node->qnode);
    }

    node->belong = NULL;
    if (node->data.k_ref != LUA_NOREF)
    {
        luaL_unref(table->L, LUA_REGISTRYINDEX, node->data.k_ref);
        node->data.k_ref = LUA_NOREF;
    }
    if (node->data.v_ref != LUA_NOREF)
    {
        luaL_unref(table->L, LUA_REGISTRYINDEX, node->data.v_ref);
        node->data.v_ref = LUA_NOREF;
    }
    if (node->data.s_ref != LUA_NOREF)
    {
        luaL_unref(table->L, LUA_REGISTRYINDEX, node->data.s_ref);
        node->data.s_ref = LUA_NOREF;
    }

    return 0;
}

static infra_map_t* _get_map(lua_State* L, int idx)
{
    infra_map_t* table = lua_touserdata(L, idx);
    table->L = L;
    return table;
}

static infra_map_t* _get_map_check(lua_State* L, int idx)
{
    infra_map_t* table = luaL_checkudata(L, idx, infra_type_name(INFRA_MAP));
    table->L = L;
    return table;
}

static void _map_clear_all_node(infra_map_t* table)
{
    ev_map_node_t* it = ev_map_begin(&table->table);
    while (it != NULL)
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);
        it = ev_map_next(it);

        _map_erase_node(table, node, 1);
    }
}

/**
 * @brief Insert key at \p kidx and value at \p vidx into map at \p dst, push
 *   iterator onto top of stack \p L.
 * @note This function does not remove key and value from stack.
 * @param[in] L     Lua VM.
 * @param[in] dst   Map index.
 * @param[in] kidx  Key index.
 * @param[in] vidx  Value index.
 * @param[in] force If key conflict, replace old value with new value.
 * @return          Boolean. 1 if success, 0 if failure.
 */
static int _map_insert(lua_State* L, int dst, int kidx, int vidx, int force)
{
    infra_map_t* table = _get_map(L, dst);

    infra_map_node_t* node = lua_newuserdata(L, sizeof(infra_map_node_t));
    node->belong = table;

    lua_pushvalue(L, -1);
    node->data.s_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, kidx);
    node->data.k_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, vidx);
    node->data.v_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if (force)
    {
        ev_map_node_t* old_node = ev_map_replace(&table->table, &node->mnode);
        if (old_node != NULL)
        {
            infra_map_node_t* orig_node = container_of(old_node, infra_map_node_t, mnode);
            _map_erase_node(table, orig_node, 0);
        }
        return 1;
    }

    if (ev_map_insert(&table->table, &node->mnode) != 0)
    {
        _map_erase_node(table, node, 0);
        lua_pop(L, 1);
        return 0;
    }
    ev_list_push_back(&table->queue, &node->qnode);

    return 1;
}

/**
 * @brief Like #_map_insert(), but do not push iterator onto stack.
 * @see #_map_insert()
 */
static int _map_insert_no_iter(lua_State* L, int dst, int kidx, int vidx, int force)
{
    int ret = _map_insert(L, dst, kidx, vidx, force);
    if (ret)
    {
        lua_pop(L, 1);
    }
    return ret;
}

/**
 * @brief Store all elements that exist in \p idx1 but not in \p idx2 into \p dst.
 */
static void _map_diff(lua_State* L, int dst, int idx1, int idx2)
{
    int sp = lua_gettop(L);
    infra_map_t* t1 = _get_map_check(L, idx1);
    infra_map_t* t2 = _get_map_check(L, idx2);

    ev_map_node_t* it = ev_map_begin(&t1->table);
    for (; it != NULL; it = ev_map_next(it))
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);

        infra_map_node_t tmp;
        tmp.data.k_ref = node->data.k_ref;

        ev_map_node_t* t2_it = ev_map_find(&t2->table, &tmp.mnode);
        if (t2_it == NULL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
            lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
            _map_insert_no_iter(L, dst, sp + 1, sp + 2, 0);
            lua_pop(L, 2);
        }
    }
}

/**
 * @brief Copy all elements from lua table at \p src into map at \p dst
 * @param[in] L     Lua VM.
 * @param[in] dst   Stack index for #infra_map_t.
 * @param[in] src   Stack index for lua table.
 * @param[in] force If key conflict, use value from \src.
 * @return          Always 1.
 */
static int _map_copy_from_table(lua_State* L, int dst, int src, int force)
{
    int sp = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, src))
    {
        if (_map_insert(L, dst, sp + 1, sp + 2, force))
        {/* Insert success, pop iterator */
            lua_pop(L, 1);
        }
        /* Pop value */
        lua_pop(L, 1);
    }

    return 1;
}

/**
 * @brief Copy all elements from infra map at \p src into map at \p dst
 * @param[in] L     Lua VM.
 * @param[in] dst   Stack index for #infra_map_t.
 * @param[in] src   Stack index for #infra_map_t.
 * @param[in] force If key conflict, use value from \src.
 * @return          Always 1.
 */
static int _map_copy_from_map(lua_State* L, int dst, int src, int force)
{
    int sp = lua_gettop(L);
    infra_map_t* src_map = _get_map(L, src);

    ev_map_node_t* it = ev_map_begin(&src_map->table);
    for (; it != NULL; it = ev_map_next(it))
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);

        if (_map_insert(L, dst, sp + 1, sp + 2, force))
        {
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
    }
    return 1;
}

/**
 * @brief Iterator all items in \p idx.
 * @param[in] L     Lua VM.
 * @param[in] idx   Object index that have __pairs metamethod.
 * @param[in] cb    Iterator callback. Return 0 to stop iterator.
 * @param[in] arg   Pass to \p cb.
 * @return          1 if success, 0 if object at \p idx does not have `__pairs` metamethod.
 */
static int _map_foreach_pairs(lua_State* L, int idx,
    int(*cb)(lua_State*, int, int, void*), void* arg)
{
    int sp = lua_gettop(L);

    int val_type = luaL_getmetafield(L, idx, "__pairs");
    if (val_type == LUA_TNIL)
    {
        return 0;
    }

    lua_pushvalue(L, idx);
    lua_call(L, 1, 3);

    /*
     * SP+1: next function
     * SP+2: table
     * SP+3: nil
     */

    int next_func = sp + 1;
    int user_obj = sp + 2;

    lua_pushvalue(L, next_func);
    lua_pushvalue(L, user_obj);
    lua_pushnil(L);

    for (;;)
    {
        /*
         * Required stack layout:
         * SP+1: next function
         * SP+2: table
         * SP+3: nil
         * SP+4: next function
         * SP+5: table
         * SP+6: key
         */
        lua_call(L, 2, 2);

        /*
         * After call:
         * SP+1: next function
         * SP+2: table
         * SP+3: nil
         * SP+4: key
         * SP+5: value
         */

        if (lua_type(L, sp + 4) == LUA_TNIL)
        {
            break;
        }

        if (cb(L, sp + 4, sp + 5, arg) == 0)
        {
            break;
        }

        /* Push key to SP+6 */
        lua_pushvalue(L, sp + 4);
        /* Replace SP+4 with next function */
        lua_pushvalue(L, next_func);
        lua_replace(L, sp + 4);
        /* Replace SP+5 with table object */
        lua_pushvalue(L, user_obj);
        lua_replace(L, sp + 5);
    }

    /* Restore stack */
    lua_settop(L, sp);

    return 1;
}

static int _map_on_copy_from_pairs(lua_State* L, int key, int val, void* arg)
{
    int* p_help = (int*)arg;
    int dst = p_help[0];
    int force = p_help[1];

    _map_insert_no_iter(L, dst, key, val, force);
    return 0;
}

/**
 * @brief Copy all elements from object at \p src into map at \p dst.
 * @param[in] L     Lua VM.
 * @param[in] dst   Stack index for #infra_map_t.
 * @param[in] src   Stack index for object which must have `__pairs` metamethod.
 * @param[in] force If key conflict, use value from \src.
 * @return          1 if success, 0 if failure.
 */
static int _map_copy_from_pairs(lua_State* L, int dst, int src, int force)
{
    int help[2] = { dst, force };
    return _map_foreach_pairs(L, src, _map_on_copy_from_pairs, help);
}

/**
 * @brief Copy generic table at \p src into \p dst.
 * @param[in] L     Lua VM.
 * @param[in] dst   Target #infra_map_t.
 * @param[in] src   Source table. Can be a lua table , a infra map or any
 *   userdata with `__pairs`  metamethod.
 * @param[in] force Force replace original value.
 * @return          1 if success, 0 if failure.
 */
static int _map_copy(lua_State* L, int dst, int src, int force)
{
    int val_type = infra_type(L, src);

    /* This is a table */
    if (val_type == LUA_TTABLE)
    {
        return _map_copy_from_table(L, dst, src, force);
    }

    /* Infra map */
    if (val_type == INFRA_MAP)
    {
        return _map_copy_from_map(L, dst, src, force);
    }

    return _map_copy_from_pairs(L, dst, src, force);
}

static int _map_pairs_next(lua_State* L)
{
    ev_map_node_t* it;
    infra_map_node_t* node;
    infra_map_t* table = _get_map(L, 1);

    lua_settop(L, 2);

    /* First key */
    if (lua_type(L, 2) == LUA_TNIL)
    {
        it = ev_map_begin(&table->table);
        if (it == NULL)
        {
            goto no_more_key;
        }

        node = container_of(it, infra_map_node_t, mnode);
        goto push_kv;
    }

    infra_map_node_t tmp;
    tmp.data.k_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    it = ev_map_find(&table->table, &tmp.mnode);
    it = ev_map_next(it);
    luaL_unref(L, LUA_REGISTRYINDEX, tmp.data.k_ref);

    if (it == NULL)
    {
        goto no_more_key;
    }

    node = container_of(it, infra_map_node_t, mnode);

push_kv:
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
    return 2;

no_more_key:
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

static int _map_compare_with_infra_map(lua_State* L, infra_map_t* t1, infra_map_t* t2)
{
    int ret = 1;
    if (ev_map_size(&t1->table) != ev_map_size(&t1->table))
    {
        return 0;
    }
    if (t1 == t2)
    {
        return 1;
    }

    int sp = lua_gettop(L);
    ev_map_node_t* it1 = ev_map_begin(&t1->table);
    ev_map_node_t* it2 = ev_map_begin(&t2->table);

    for (; it1 != NULL; it1 = ev_map_next(it1), it2 = ev_map_next(it2))
    {
        infra_map_node_t* node1 = container_of(it1, infra_map_node_t, mnode);
        infra_map_node_t* node2 = container_of(it2, infra_map_node_t, mnode);

        lua_rawgeti(L, LUA_REGISTRYINDEX, node1->data.k_ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node2->data.k_ref);
        if (lua_compare(L, sp + 1, sp + 2, LUA_OPEQ) == 0)
        {
            ret = 0;
            break;
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, node1->data.v_ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node2->data.v_ref);
        if (lua_compare(L, sp + 3, sp + 4, LUA_OPEQ) == 0)
        {
            ret = 0;
            break;
        }

        lua_pop(L, 4);
    }

    lua_settop(L, sp);
    return ret;
}

static int _map_compare_map_with_lua_table(lua_State* L, infra_map_t* t1, int t2)
{
    int ret = 1;
    ev_map_node_t* it = ev_map_begin(&t1->table);
    for (; ret && it != NULL; it = ev_map_next(it))
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
        lua_gettable(L, t2);
        ret = lua_compare(L, -1, -2, LUA_OPEQ);
        lua_pop(L, 2);
    }
    return ret;
}

static int _map_compare_with_generic(lua_State* L, infra_map_t* t1, int t2)
{
    int ret = 1;
    int sp = lua_gettop(L);

    if (luaL_getmetafield(L, t2, "__index") == LUA_TNIL)
    {
        return infra_typeerror(L, t2, "__index");
    }
    /* SP+1: __index */

    ev_map_node_t* it = ev_map_begin(&t1->table);
    for (; ret && it != NULL; it = ev_map_next(it))
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);

        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
        lua_pushvalue(L, sp + 1);
        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
        lua_call(L, 1, 1);

        /*
         * SP+1: __index
         * SP+2: value from t1
         * SP+3: value from t2
         */

        ret = lua_compare(L, sp + 2, sp + 3, LUA_OPEQ);

        lua_pop(L, 2);
    }

    return ret;
}

/**
 * @brief Find \p key in \p map.
 * @return      Map node.
 */
static infra_map_node_t* _map_find_iter(lua_State* L, infra_map_t* table, int key)
{
    infra_map_node_t tmp;

    lua_pushvalue(L, key);
    tmp.data.k_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_map_node_t* it = ev_map_find(&table->table, &tmp.mnode);
    luaL_unref(L, LUA_REGISTRYINDEX, tmp.data.k_ref);

    return it != NULL ? container_of(it, infra_map_node_t, mnode) : NULL;
}

static int _map_on_generic_pair(lua_State* L, int key, int value, void* arg)
{
    void** p_helper = arg;
    infra_map_t* t2 = (infra_map_t*)p_helper[0];
    int* p_ret = (int*)p_helper[1];
    int ret = 1;

    infra_map_node_t* node = _map_find_iter(L, t2, key);
    if (node == NULL)
    {
        ret = 0;
        goto finish;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
    ret = lua_compare(L, value, -1, LUA_OPEQ);
    lua_pop(L, 1);

finish:
    *p_ret = ret;
    return ret;
}

static int _map_compare_generic_wit_map(lua_State* L, int t1, infra_map_t* t2)
{
    void* helper[2] = { t2, NULL };
    if (_map_foreach_pairs(L, t1, _map_on_generic_pair, helper) == 0)
    {
        return infra_typeerror(L, t1, "__pairs");
    }

    return (int)(intptr_t)helper[1];
}

static int _map_compare_lua_table_with_map(lua_State* L, int t1, infra_map_t* t2)
{
    int ret = 1;
    int sp = lua_gettop(L);

    lua_pushnil(L);
    while (ret && lua_next(L, t1) != 0)
    {
        infra_map_node_t* node = _map_find_iter(L, t2, sp + 1);
        if (node == NULL)
        {
            ret = 0;
            break;
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
        ret = lua_compare(L, sp + 2, sp + 3, LUA_OPEQ);
        lua_pop(L, 2);
    }

    lua_settop(L, sp);
    return ret;
}

static int _map_op_assign(lua_State* L)
{
    return _map_insert(L, 1, 2, 3, 1);
}

static int _map_op_insert(lua_State* L)
{
    return _map_insert(L, 1, 2, 3, 0);
}

static int _map_op_begin(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);
    ev_map_node_t* it = ev_map_begin(&table->table);
    if (it == NULL)
    {
        return 0;
    }

    infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.s_ref);
    return 1;
}

static int _map_op_next(lua_State* L)
{
    infra_map_node_t* node = lua_touserdata(L, 2);
    if (node == NULL)
    {
        return 0;
    }

    ev_map_node_t* it = ev_map_next(&node->mnode);
    if (it == NULL)
    {
        return 0;
    }

    node = container_of(it, infra_map_node_t, mnode);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.s_ref);
    return 1;
}

static int _map_op_erase(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);
    infra_map_node_t* node = lua_touserdata(L, 2);
    if (node != NULL && node->belong != NULL)
    {
        _map_erase_node(table, node, 1);
    }

    return 0;
}

static int _map_op_size(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);
    lua_pushinteger(L, ev_map_size(&table->table));
    return 1;
}

static int _map_op_clear(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);
    _map_clear_all_node(table);
    return 0;
}

static int _map_op_clone(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_new(L);
    _map_copy(L, sp + 1, 1, 0);
    return 1;
}

static int _map_op_copy(lua_State* L)
{
    int force = lua_toboolean(L, 3);
    _map_copy(L, 1, 2, force);
    return 0;
}

static int _map_op_equal(lua_State* L)
{
    lua_pushboolean(L, infra_map_compare(L, 1, 2));
    return 1;
}

static int _map_meta_gc(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);

    _map_clear_all_node(table);

    return 0;
}

static int _map_meta_pairs(lua_State* L)
{
    lua_pushcfunction(L, _map_pairs_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int _map_meta_len(lua_State* L)
{
    return _map_op_size(L);
}

static int _map_meta_eq(lua_State* L)
{
    return _map_op_equal(L);
}

static int _map_meta_index(lua_State* L)
{
    infra_map_t* table = _get_map(L, 1);
    infra_map_node_t* node = _map_find_iter(L, table, 2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
    return 1;
}

static int _map_meta_newindex(lua_State* L)
{
    _map_op_assign(L);
    lua_pop(L, 1);
    return 0;
}

/**
 * @brief Union of Sets
 */
static int _map_meta_bor(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_new(L);
    _map_copy(L, sp + 1, 1, 0);
    _map_copy(L, sp + 1, 2, 0);

    return 1;
}

/**
 * Intersection of Sets
 */
static int _map_meta_band(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_new(L);
    infra_map_t* t1 = _get_map(L, 1);
    infra_map_t* t2 = _get_map_check(L, 2);

    ev_map_node_t* it = ev_map_begin(&t1->table);
    for (; it != NULL; it = ev_map_next(it))
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, mnode);

        infra_map_node_t tmp;
        tmp.data.k_ref = node->data.k_ref;

        ev_map_node_t* t2_it = ev_map_find(&t2->table, &tmp.mnode);
        if (t2_it != NULL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.k_ref);
            lua_rawgeti(L, LUA_REGISTRYINDEX, node->data.v_ref);
            _map_insert_no_iter(L, sp + 1, sp + 2, sp + 3, 0);
            lua_pop(L, 2);
        }
    }

    return 1;
}

/**
 * @brief Set Difference.
 */
static int _map_meta_sub(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_new(L);
    _map_diff(L, sp + 1, 1, 2);
    return 1;
}

static int _map_meta_add(lua_State* L)
{
    return _map_meta_bor(L);
}

/**
 * @brief Complement of Sets
 */
static int _map_meta_pow(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_new(L);
    _map_diff(L, sp + 1, 1, 2);
    _map_diff(L, sp + 1, 2, 1);
    return 1;
}

static void _table_set_meta(lua_State* L)
{
    static const luaL_Reg s_map_meta[] = {
        { "__eq",       _map_meta_eq },
        { "__add",      _map_meta_add },
        { "__sub",      _map_meta_sub },
        { "__bor",      _map_meta_bor },
        { "__band",     _map_meta_band },
        { "__pow",      _map_meta_pow },
        { "__index",    _map_meta_index },
        { "__newindex", _map_meta_newindex },
        { "__pairs",    _map_meta_pairs },
        { "__len",      _map_meta_len },
        { "__gc",       _map_meta_gc },
        { NULL,         NULL },
    };
    static const luaL_Reg s_map_method[] = {
        { "copy",       _map_op_copy },
        { "clone",      _map_op_clone },
        { "clear",      _map_op_clear },
        { "insert",     _map_op_insert },
        { "assign",     _map_op_assign },
        { "begin",      _map_op_begin },
        { "next",       _map_op_next },
        { "erase",      _map_op_erase },
        { "size",       _map_op_size },
        { "equal",      _map_op_equal },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(L, infra_type_name(INFRA_MAP)) != 0)
    {
        luaL_setfuncs(L, s_map_meta, 0);

        /* metatable.__index = s_map_method */
        luaL_newlib(L, s_map_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
}

int infra_map_new(lua_State* L)
{
    infra_map_t* table = lua_newuserdata(L, sizeof(infra_map_t));

    ev_map_init(&table->table, _on_cmp_node, table);
    ev_list_init(&table->queue);

    /*
     * It is not safe to record LuaVM here, because this function may be called
     * in lua thread.
     */
    table->L = NULL;

    _table_set_meta(L);
    return 1;
}

int infra_new_map(lua_State* L)
{
    int sp = lua_gettop(L);
    int val_type = infra_type(L, 1);
    infra_map_new(L);

    if (val_type != LUA_TNIL)
    {
        _map_copy(L, sp + 1, 1, 0);
    }

    return 1;
}

int infra_map_compare(lua_State* L, int idx1, int idx2)
{
    infra_map_t* table = _get_map_check(L, idx1);
    int val_type = infra_type(L, idx2);

    /* This is infra map */
    if (val_type == INFRA_MAP)
    {
        return _map_compare_with_infra_map(L, table, lua_touserdata(L, idx2));
    }

    if (val_type == LUA_TTABLE)
    {
        return _map_compare_map_with_lua_table(L, table, idx2) &&
            _map_compare_lua_table_with_map(L, idx2, table);
    }

    return _map_compare_with_generic(L, table, idx2) &&
        _map_compare_generic_wit_map(L, idx2, table);
}
