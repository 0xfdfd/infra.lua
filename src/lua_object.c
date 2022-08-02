#include "lua_object.h"
#include "utils/map.h"
#include <string.h>
#include <uv.h>

typedef struct infra_type_record_s
{
    ev_map_node_t   node;   /**< Map node */
    uint64_t        hname;  /**< Hash of name. */
    int             value;  /**< Value of #infra_object_type_t */
    const char*     name;   /**< Type name as c string. */
} infra_type_record_t;

typedef struct infra_runtime
{
    uv_mutex_t      glock;          /**< Global lock */
    ev_list_t       vm_list;        /**< Registered VM list */
    ev_list_t       obj_list;       /**< Orphan object list */
    ev_map_t        type_table;     /**< Find type by meta table name */
} infra_runtime_t;

static infra_runtime_t s_infra_runtime;

static infra_type_record_t s_infra_type_list[] = {
#define EXPAND_LUA_OBJECT_TABLE_AS_LIST(name)   { EV_MAP_NODE_INIT, 0, name, "__" #name},
    LUA_OBJECT_TABLE(EXPAND_LUA_OBJECT_TABLE_AS_LIST)
#undef EXPAND_LUA_OBJECT_TABLE_AS_LIST
};

static int _infra_on_cmp_hname(const ev_map_node_t* key1,
                               const ev_map_node_t* key2,
                               void* arg)
{
    (void)arg;
    infra_type_record_t* rec_1 = container_of(key1, infra_type_record_t, node);
    infra_type_record_t* rec_2 = container_of(key2, infra_type_record_t, node);
    if (rec_1->hname == rec_2->hname)
    {
        return 0;
    }
    return rec_1->hname < rec_2->hname ? -1 : 1;
}

static uint64_t _bkdr(const void* data, size_t size, uint64_t seed)
{
    const uint8_t* buffer = data;
    size_t i;
    for (i = 0; i < size; i++)
    {
        seed = seed * 131 + buffer[i];
    }
    return seed;
}

static void _infra_runtime_on_init(void)
{
    uv_mutex_init(&s_infra_runtime.glock);
    ev_list_init(&s_infra_runtime.vm_list);
    ev_list_init(&s_infra_runtime.obj_list);
    ev_map_init(&s_infra_runtime.type_table, _infra_on_cmp_hname, NULL);

    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_infra_type_list); i++)
    {
        s_infra_type_list[i].hname = _bkdr(s_infra_type_list[i].name,
            strlen(s_infra_type_list[i].name), 0);
        ev_map_insert(&s_infra_runtime.type_table, &s_infra_type_list[i].node);
    }
}

static void _infra_object_initialize_once(void)
{
    static uv_once_t once_token = UV_ONCE_INIT;
    uv_once(&once_token, _infra_runtime_on_init);
}

static void _runtime_move_to_orphan_list(infra_vm_runtime_t* rt)
{
    ev_list_node_t* it;
    while ((it = ev_list_pop_front(&rt->object_list)) != NULL)
    {
        infra_object_t* object = container_of(it, infra_object_t, node);

        /* Disconnect runtime */
        object->vm = NULL;

        /* Move ownership to root runtime */
        uv_mutex_lock(&s_infra_runtime.glock);
        ev_list_push_back(&s_infra_runtime.obj_list, &object->node);
        uv_mutex_unlock(&s_infra_runtime.glock);
    }
}

static int _runtime_gc(lua_State* L)
{
    infra_vm_runtime_t* rt = lua_touserdata(L, 1);

    _runtime_move_to_orphan_list(rt);

    /* Remove self */
    uv_mutex_lock(&s_infra_runtime.glock);
    ev_list_erase(&s_infra_runtime.vm_list, &rt->node);
    uv_mutex_unlock(&s_infra_runtime.glock);

    return 0;
}

static void _infra_set_runtime_meta(lua_State *L)
{
    static const luaL_Reg s_runtime_meta[] = {
        { "__gc",   _runtime_gc },
        { NULL, NULL }
    };

    if (luaL_newmetatable(L, "__infra_vm_runtime") != 0)
    {
        luaL_setfuncs(L, s_runtime_meta, 0);
    }
    lua_setmetatable(L, -2);
}

static infra_vm_runtime_t* _infra_get_vm_instance(lua_State *L)
{
    if (lua_getglobal(L, "__infra_vm_runtime") != LUA_TNIL)
    {
        infra_vm_runtime_t* rt = lua_touserdata(L, -1);

        /* This is a global value, it is safe to pop it. */
        lua_pop(L, 1);

        return rt;
    }

    /* Pop nil from #lua_getglobal() */
    lua_pop(L, 1);

    /* Create VM global runtime */
    infra_vm_runtime_t* rt = lua_newuserdata(L, sizeof(infra_vm_runtime_t));
    ev_list_init(&rt->object_list);
    _infra_set_runtime_meta(L);
    lua_setglobal(L, "__infra_vm_runtime");

    /* Record VM global runtime */
    uv_mutex_lock(&s_infra_runtime.glock);
    ev_list_push_back(&s_infra_runtime.vm_list, &rt->node);
    uv_mutex_unlock(&s_infra_runtime.glock);

    return rt;
}

void infra_init_object(infra_object_t* object, lua_State *L, infra_object_type_t type)
{
    _infra_object_initialize_once();

    object->type = type;
    object->vm = _infra_get_vm_instance(L);
    ev_list_push_back(&object->vm->object_list, &object->node);
}

void infra_exit_object(infra_object_t* object)
{
    if (object->vm != NULL)
    {/* In lua vm list */
        ev_list_erase(&object->vm->object_list, &object->node);
    }
    else
    {/* In global list */
        uv_mutex_lock(&s_infra_runtime.glock);
        ev_list_erase(&s_infra_runtime.obj_list, &object->node);
        uv_mutex_unlock(&s_infra_runtime.glock);
    }
}

const char* infra_type_name(int type)
{
#define EXPAND_LUA_OBJECT_TABLE_AS_TYPE_NAME(name)  \
    case (name): return "__" #name;

    _infra_object_initialize_once();

    switch (type)
    {
    LUA_OBJECT_TABLE(EXPAND_LUA_OBJECT_TABLE_AS_TYPE_NAME)
    default:
        return lua_typename(NULL, type);
    }

#undef EXPAND_LUA_OBJECT_TABLE_AS_TYPE_NAME
}

int infra_type(lua_State* L, int idx)
{
    _infra_object_initialize_once();

    int ret = lua_type(L, idx);

    if (ret != LUA_TUSERDATA)
    {
        return ret;
    }

    /* Get metatable name */
    ret = luaL_getmetafield(L, idx, "__name");
    if (ret != LUA_TSTRING)
    {
        ret = LUA_TUSERDATA;
        goto finish;
    }

    size_t size;
    const char* type_name = lua_tolstring(L, -1, &size);

    infra_type_record_t tmp;
    tmp.hname = _bkdr(type_name, size, 0);

    ev_map_node_t* it = ev_map_find(&s_infra_runtime.type_table, &tmp.node);
    if (it == NULL)
    {
        /* Unknown user data type */
        ret = LUA_TUSERDATA;
        goto finish;
    }

    infra_type_record_t* rec = container_of(it, infra_type_record_t, node);
    ret = rec->value;

finish:
    lua_pop(L, 1);  /* Remain stack balance */
    return ret;
}
