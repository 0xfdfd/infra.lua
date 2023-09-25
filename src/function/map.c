#include "__init__.h"
#include "utils/map.h"
#include <assert.h>

#define INFRA_MAP_NAME  "__infra_map"

typedef struct infra_map_node
{
    ev_map_node_t   node;
    int             refk;
    int             refv;
} infra_map_node_t;

typedef struct infra_map
{
    ev_map_t        root;
    lua_State*      L;
} infra_map_t;

static int _infra_map_cmp(const ev_map_node_t* key1, const ev_map_node_t* key2, void* arg)
{
    infra_map_t* self = arg;
    lua_State* L = self->L;

    infra_map_node_t* n1 = container_of(key1, infra_map_node_t, node);
    infra_map_node_t* n2 = container_of(key2, infra_map_node_t, node);

    lua_pushcfunction(L, infra_f_compare.addr);
    lua_rawgeti(L, LUA_REGISTRYINDEX, n1->refk);
    lua_rawgeti(L, LUA_REGISTRYINDEX, n2->refk);
    lua_call(L, 2, 1);

    int ret = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    return ret;
}

static void _infra_map_erase_node(lua_State* L, infra_map_t* self, infra_map_node_t* node)
{
    ev_map_erase(&self->root, &node->node);

    luaL_unref(L, LUA_REGISTRYINDEX, node->refk);
    node->refk = LUA_NOREF;

    luaL_unref(L, LUA_REGISTRYINDEX, node->refv);
    node->refv = LUA_NOREF;

    free(node);
}

static int _infra_map_gc(lua_State* L)
{
    infra_map_t* self = lua_touserdata(L, 1);

    ev_map_node_t* it = ev_map_begin(&self->root);
    while (it != NULL)
    {
        infra_map_node_t* node = container_of(it, infra_map_node_t, node);
        it = ev_map_next(it);

        _infra_map_erase_node(L, self, node);
    }

    return 0;
}

static int _infra_map_size(lua_State* L)
{
    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);

    size_t size = ev_map_size(&self->root);
    lua_pushinteger(L, size);
    return 1;
}

static int _infra_map_insert(lua_State* L)
{
    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);
    self->L = L;

    infra_map_node_t* node = malloc(sizeof(infra_map_node_t));
    if (node == NULL)
    {
        return luaL_error(L, "out of memory.");
    }

    lua_pushvalue(L, 2);
    node->refk = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 3);
    node->refv = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_map_node_t* orig = ev_map_insert(&self->root, &node->node);
    if (orig != NULL)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, node->refk);
        node->refk = LUA_NOREF;
        luaL_unref(L, LUA_REGISTRYINDEX, node->refv);
        node->refv = LUA_NOREF;
        free(node);

        lua_pushboolean(L, 0);
    }
    else
    {
        lua_pushboolean(L, 1);
    }

    return 1;
}

static int _infra_map_replace(lua_State* L)
{
    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);
    self->L = L;

    infra_map_node_t* node = malloc(sizeof(infra_map_node_t));
    if (node == NULL)
    {
        return luaL_error(L, "out of memory.");
    }

    lua_pushvalue(L, 2);
    node->refk = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, 3);
    node->refv = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_map_node_t* orig = ev_map_insert(&self->root, &node->node);
    if (orig == NULL)
    {
        return 0;
    }

    infra_map_node_t* orig_node = container_of(orig, infra_map_node_t, node);
    _infra_map_erase_node(L, self, orig_node);

    orig = ev_map_insert(&self->root, &node->node);
    assert(orig == NULL);

    return 0;
}

static int _infra_map_erase(lua_State* L)
{
    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);
    self->L = L;

    infra_map_node_t tmp_node;
    lua_pushvalue(L, 2);
    tmp_node.refk = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_map_node_t* it = ev_map_find(&self->root, &tmp_node.node);
    luaL_unref(L, LUA_REGISTRYINDEX, tmp_node.refk);

    if (it == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    infra_map_node_t* node = container_of(it, infra_map_node_t, node);
    _infra_map_erase_node(L, self, node);

    lua_pushboolean(L, 1);
    return 1;
}

static int _infra_map_pairs_next(lua_State* L)
{
    ev_map_node_t* it = NULL;
    infra_map_node_t* node = NULL;

    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);
    self->L = L;

    if (lua_type(L, 2) == LUA_TNIL)
    {
        if ((it = ev_map_begin(&self->root)) == NULL)
        {
            goto return_nil;
        }

        goto return_node;
    }

    infra_map_node_t tmp_node;
    lua_pushvalue(L, 2);
    tmp_node.refk = luaL_ref(L, LUA_REGISTRYINDEX);

    it = ev_map_find_upper(&self->root, &tmp_node.node);
    luaL_unref(L, LUA_REGISTRYINDEX, tmp_node.refk);

    if (it == NULL)
    {
        goto return_nil;
    }

return_node:
    node = container_of(it, infra_map_node_t, node);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->refk);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->refv);
    return 2;

return_nil:
    lua_pushnil(L);
    return 1;
}

static int _infra_map_meta_pairs(lua_State* L)
{
    lua_pushcfunction(L, _infra_map_pairs_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int _infra_map_pairs(lua_State* L)
{
    luaL_checkudata(L, 1, INFRA_MAP_NAME);
    return _infra_map_meta_pairs(L);
}

static int _infra_map_find(lua_State* L)
{
    infra_map_t* self = luaL_checkudata(L, 1, INFRA_MAP_NAME);
    self->L = L;

    infra_map_node_t tmp_node;
    lua_pushvalue(L, 2);
    tmp_node.refk = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_map_node_t* it = ev_map_find(&self->root, &tmp_node.node);
    luaL_unref(L, LUA_REGISTRYINDEX, tmp_node.refk);

    if (it == NULL)
    {
        lua_pushboolean(L, 0);
        lua_pushnil(L);
        return 2;
    }

    infra_map_node_t* node = container_of(it, infra_map_node_t, node);

    lua_pushboolean(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, node->refv);

    return 2;
}

static int _infra_map_copy_from_table(lua_State* L, int dst, int src)
{
    int sp = lua_gettop(L);

    lua_pushnil(L); /* key:sp+1 */
    while (lua_next(L, src) != 0) /* value:sp+2 */
    {
        lua_pushcfunction(L, _infra_map_insert);
        lua_pushvalue(L, dst);
        lua_pushvalue(L, sp + 1);
        lua_pushvalue(L, sp + 2);
        lua_call(L, 3, 0);

        lua_pop(L, 1);
    }

    return 0;
}

static int _infra_map_copy_from_pairs(lua_State* L, int dst, int src)
{
    int sp = lua_gettop(L);
    if (luaL_callmeta(L, src, "__pairs") == 0)
    {
        return luaL_error(L, "no metamethod `__pairs`.");
    }
    lua_settop(L, sp + 3);

    while (1)
    {
        lua_pushvalue(L, sp + 1);
        lua_pushvalue(L, sp + 2);
        lua_pushvalue(L, sp + 3);
        lua_call(L, 2, 2); // k:sp+4, v:sp+5

        lua_pushcfunction(L, _infra_map_insert); // sp+6
        lua_pushvalue(L, dst); // sp+7
        lua_pushvalue(L, sp + 4); // sp+8
        lua_pushvalue(L, sp + 5); // sp+9
        lua_call(L, 3, 0); // sp+5

        lua_replace(L, sp + 3); // sp+4
        lua_replace(L, sp + 2); // sp+3
    }

    lua_settop(L, sp);
    return 0;
}

static int _infra_map_smart_copy(lua_State* L, int dst, int src)
{
    src = lua_absindex(L, src);
    dst = lua_absindex(L, dst);

    if (lua_type(L, src) == LUA_TTABLE)
    {
        return _infra_map_copy_from_table(L, dst, src);
    }

    int type = luaL_getmetafield(L, src, "__pairs");
    if (type == LUA_TNIL)
    {
        return 0;
    }
    lua_pop(L, 1);

    if (type == LUA_TFUNCTION)
    {
        return _infra_map_copy_from_pairs(L, dst, src);
    }
    return 0;
}

static int _infra_new_map(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_map_t* self = lua_newuserdata(L, sizeof(infra_map_t));

    self->L = L;
    ev_map_init(&self->root, _infra_map_cmp, self);

    static const luaL_Reg s_meta[] = {
        { "__gc",       _infra_map_gc },
        { "__pairs",    _infra_map_meta_pairs },
        { NULL,         NULL },
    };
    static const luaL_Reg s_method[] = {
        { "size",       _infra_map_size },
        { "insert",     _infra_map_insert },
        { "replace",    _infra_map_replace },
        { "erase",      _infra_map_erase },
        { "find",       _infra_map_find },
        { "pairs",      _infra_map_pairs },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(L, INFRA_MAP_NAME) != 0)
    {
        luaL_setfuncs(L, s_meta, 0);

        /* metatable.__index = s_method */
        luaL_newlib(L, s_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);

    if (sp >= 1)
    {
        _infra_map_smart_copy(L, -1, 1);
    }

    return 1;
}

const infra_lua_api_t infra_f_map = {
"make_map", _infra_new_map, 0,
"Create a new empty map.",

"[SYNOPSIS]\n"
"map make_map()\n"
"\n"
"[DESCRIPTION]\n"
"Create a new empty map. A map is a red-black tree that able to contains strings,\n"
"booleans, integers as it's key and anything as it's value.\n"
"\n"
"A map have following metamethod:\n"
"  integer map:size()\n"
"    Return the number of elements.\n"
"  boolean map:insert(key, value)\n"
"    Insert key and value into map. If a key is exist, return false.\n"
"  map:replace(key, value)\n"
"    Insert key and value into map, replace any existing key and value.\n"
"  boolean map:erase(key)\n"
"    Erase key-value pair from map. If the key is not exist, return false.\n"
"  boolean,any map:find(key)\n"
"    Find the matching value for the key. If found, the first return value is\n"
"    true, the second is the associated value. If not found, return false and\n"
"    nil.\n"
"  map:pairs()\n"
"    Use in `for k,v in map:pairs() do ... end`. This is mainly for lua5.1 and\n"
"    luajit. If you are using lua5.2 and above, just use normal `pairs()` to\n"
"    iterate over all key-value pairs.\n"
};
