#include "lua_dump.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "utils/map.h"

typedef struct table_rec
{
    ev_map_node_t   node;
    void*           addr;
} table_rec_t;

typedef ev_map_t table_path_t;

static void _dump_value_as_string_at_index(table_path_t* path,
    luaL_Buffer* buf, lua_State* L, int idx, int level, int isindent);

static void _dump_add_prefix(luaL_Buffer* buf, int level)
{
    int i;
    for (i = 0; i < level; i++)
    {
        luaL_addstring(buf, "    ");
    }
}

static void _dump_table_as_string(table_path_t* path, luaL_Buffer* buf,
    lua_State* L, int idx, int level)
{
    table_rec_t* rec = malloc(sizeof(table_rec_t));
    assert(rec != NULL);
    rec->addr = (void*)lua_topointer(L, idx);

    if (ev_map_insert(path, &rec->node) != 0)
    {/* duplicate table */
        free(rec);
        return;
    }

    luaL_addstring(buf, "{\n");

    int top = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, idx) != 0)
    {
        _dump_value_as_string_at_index(path, buf, L, top + 1, level + 1, 1);
        luaL_addstring(buf, " = ");
        _dump_value_as_string_at_index(path, buf, L, top + 2, level + 1, 0);
        luaL_addstring(buf, ",\n");

        lua_pop(L, 1);
    }

    _dump_add_prefix(buf, level);
    luaL_addstring(buf, "}");
}

static void _dump_value_as_string_at_index(table_path_t* path,
    luaL_Buffer* buf, lua_State* L, int idx, int level, int isindent)
{
    char buffer[64];

    if (isindent)
    {
        _dump_add_prefix(buf, level);
    }

    int val_type = lua_type(L, idx);
    switch(val_type)
    {
    case LUA_TNIL:
        luaL_addstring(buf, "<NIL>");
        break;

    case LUA_TNUMBER:
        if (lua_isinteger(L, idx))
        {
            lua_Integer val = lua_tointeger(L, idx);
            snprintf(buffer, sizeof(buffer), LUA_INTEGER_FMT, val);
        }
        else
        {
            lua_Number val = lua_tonumber(L, idx);
            snprintf(buffer, sizeof(buffer), LUA_NUMBER_FMT, val);
        }
        luaL_addstring(buf, buffer);
        break;

    case LUA_TBOOLEAN:
        luaL_addstring(buf, lua_toboolean(L, idx) ? "<BOOLEAN>:TRUE" : "<BOOLEAN>:FALSE");
        break;

    case LUA_TSTRING:
        luaL_addstring(buf, lua_tostring(L, idx));
        break;

    case LUA_TTABLE:
        snprintf(buffer, sizeof(buffer), "<TABLE:%p>", lua_topointer(L, idx));
        luaL_addstring(buf, buffer);
        _dump_table_as_string(path, buf, L, idx, level);
        break;

    case LUA_TFUNCTION:
        if (lua_iscfunction(L, idx))
        {
            snprintf(buffer, sizeof(buffer), "<C_FUNCTION:%p>", lua_tocfunction(L, idx));
            luaL_addstring(buf, buffer);
        }
        else
        {
            lua_Debug ar;
            memset(&ar, 0, sizeof(ar));

            lua_pushvalue(L, idx);
            lua_getinfo(L, ">nS", &ar);

            snprintf(buffer, sizeof(buffer), "<LUA_FUNCTION:%p>", lua_topointer(L, idx));
            luaL_addstring(buf, buffer);
            luaL_addstring(buf, ar.source);

            snprintf(buffer, sizeof(buffer), ":%d", ar.linedefined);
            luaL_addstring(buf, buffer);
        }
        break;

    case LUA_TUSERDATA:
        snprintf(buffer, sizeof(buffer), "<USERDATA:%p>", lua_touserdata(L, idx));
        luaL_addstring(buf, buffer);
        break;

    case LUA_TTHREAD:
        snprintf(buffer, sizeof(buffer), "<THREAD:%p>", lua_topointer(L, idx));
        luaL_addstring(buf, buffer);
        break;

    case LUA_TLIGHTUSERDATA:
        snprintf(buffer, sizeof(buffer), "<LIGHTUSERDATA:%p>", lua_touserdata(L, idx));
        luaL_addstring(buf, buffer);
        break;

    default:
        snprintf(buffer, sizeof(buffer), "<UNKNOWN:%p>", lua_topointer(L, idx));
        luaL_addstring(buf, buffer);
        break;
    }
}

static int _on_cmp_table(const ev_map_node_t* key1, const ev_map_node_t* key2, void* arg)
{
    (void)arg;
    table_rec_t* rec_1 = container_of(key1, table_rec_t, node);
    table_rec_t* rec_2 = container_of(key2, table_rec_t, node);

    if (rec_1->addr == rec_2->addr)
    {
        return 0;
    }
    return (char*)rec_1->addr < (char*)rec_2->addr ? -1 : 1;
}

static void _cleanup_table_path(table_path_t* path)
{
    ev_map_node_t* it;
    while ((it = ev_map_begin(path)) != NULL)
    {
        table_rec_t* rec = container_of(it, table_rec_t, node);
        ev_map_erase(path, it);

        free(rec);
    }
}

int infra_dump_value_as_string(lua_State* L)
{
    luaL_Buffer buf;
    luaL_buffinit(L, &buf);

    table_path_t path;
    ev_map_init(&path, _on_cmp_table, NULL);

    _dump_value_as_string_at_index(&path, &buf, L, 1, 0, 1);
    luaL_pushresult(&buf);

    _cleanup_table_path(&path);

    return 1;
}
