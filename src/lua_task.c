#include "lua_task.h"
#include "lua_errno.h"
#include "utils/list.h"
#include "utils/map.h"
#include <stdlib.h>
#include <assert.h>
#include <uv.h>

#define INFRA_SCHEDULER "infra_scheduler"

struct infra_scheduler;
typedef struct infra_scheduler infra_scheduler_t;

typedef struct coroutine_record
{
    ev_map_node_t           co_node;    /**< Node for record */
    ev_list_node_t          sc_node;    /**< Node for schedule */

    infra_scheduler_t*      scheduler;  /**< Which scheduler belong to */

    struct
    {
        lua_State*          co_thread;
        int                 co_ref_key;
    } data;

    /**
     * The addons have strict close order. Call #_task_do_close_coroutine
     * to ensure this stop is right.
     */
    struct
    {
        uv_idle_t           execution;
        uv_timer_t          timer;
    } addon;
} coroutine_record_t;

struct infra_scheduler
{
    ev_map_t                co_map;

    ev_list_t               busy_queue;
    ev_list_t               wait_queue;
    ev_list_t               dead_queue;

    uv_loop_t               loop;           /**< Event loop */

    size_t                  cnt_dead;       /**< Counter for dead coroutine */
    lua_State*              host_vm;
};

static void _task_on_close_uv_timer(uv_handle_t *handle)
{
    uv_timer_t* timer = (uv_timer_t*)handle;
    coroutine_record_t* rec = container_of(timer, coroutine_record_t, addon.timer);
    infra_scheduler_t* scheduler = rec->scheduler;

    ev_list_erase(&scheduler->dead_queue, &rec->sc_node);
    free(rec);
}

static void _task_on_close_uv_idle(uv_handle_t* handle)
{
    uv_idle_t* p_idle = (uv_idle_t*)handle;
    coroutine_record_t* rec = container_of(p_idle, coroutine_record_t, addon.execution);

    uv_close((uv_handle_t *) &rec->addon.timer, _task_on_close_uv_timer);
}

/**
 * @brief Before call this function, make sure coroutine is in dead queue.
 *
 * The coroutine is not closed immediately.
 *
 * @param rec   The coroutine to be close.
 */
static void _task_do_close_coroutine(coroutine_record_t* rec)
{
    /* Release Lua resource */
    if (rec->data.co_ref_key != LUA_NOREF)
    {
        luaL_unref(rec->data.co_thread, LUA_REGISTRYINDEX, rec->data.co_ref_key);
        rec->data.co_ref_key = LUA_NOREF;
    }
    rec->data.co_thread = NULL;

    uv_close((uv_handle_t *) &rec->addon.execution, _task_on_close_uv_idle);
}

static void _task_cleanup_dead_thread(infra_scheduler_t* scheduler)
{
    ev_list_node_t* it = ev_list_begin(&scheduler->dead_queue);
    for (; it != NULL; it = ev_list_next(it))
    {
        coroutine_record_t* rec = container_of(it, coroutine_record_t, sc_node);
        _task_do_close_coroutine(rec);
    }
}

static int _task_finalizer(lua_State* L)
{
    infra_scheduler_t* scheduler = lua_touserdata(L, 1);
    assert(scheduler != NULL);

    /* Close all alive coroutine */
    scheduler->host_vm = L;
    ev_list_migrate(&scheduler->dead_queue, &scheduler->busy_queue);
    ev_list_migrate(&scheduler->dead_queue, &scheduler->wait_queue);
    _task_cleanup_dead_thread(scheduler);

    /* Ensure all handle is closed */
    uv_run(&scheduler->loop, UV_RUN_DEFAULT);

    /* Now all resource is closed */
    scheduler->host_vm = NULL;
    uv_loop_close(&scheduler->loop);

    return 0;
}

static const luaL_Reg s_task_meta[] = {
    { "__gc", _task_finalizer },
    { NULL, NULL }
};

static int _task_on_cmp(const ev_map_node_t* key1,
                        const ev_map_node_t* key2, void* arg)
{
    (void)arg;
    coroutine_record_t* rec_1 = container_of(key1, coroutine_record_t, co_node);
    coroutine_record_t* rec_2 = container_of(key2, coroutine_record_t, co_node);
    if (rec_1->data.co_thread == rec_2->data.co_thread)
    {
        return 0;
    }
    return rec_1->data.co_thread < rec_2->data.co_thread ? -1 : 1;
}

static void _task_set_metatable(lua_State* L)
{
    if (luaL_newmetatable(L, INFRA_SCHEDULER) != 0)
    {
        luaL_setfuncs(L, s_task_meta, 0);
    }

    lua_setmetatable(L, -2);
}

static void _task_on_coroutine_run(uv_idle_t *handle)
{
    coroutine_record_t* rec = container_of(handle, coroutine_record_t, addon.execution);
    infra_scheduler_t* scheduler = rec->scheduler;

    int n_results = 0;
    int errcode = lua_resume(rec->data.co_thread, scheduler->host_vm, 0, &n_results);

    if (errcode == LUA_YIELD)
    {
        /*
         * Do nothing if coroutine yields:
         * 1. If somebody call `task_wait()` in this thread, then this uv_idle_t
         *   is already stopped.
         * 2. If no one calls `task_wait()`, this thread need to execute again.
         */
        return;
    }

    /* finishes execution with or without errors */
    ev_list_erase(&scheduler->busy_queue, &rec->sc_node);
    ev_list_push_back(&scheduler->dead_queue, &rec->sc_node);
    _task_do_close_coroutine(rec);
    return;
}

static void _task_set_as_ready(infra_scheduler_t* scheduler,
    coroutine_record_t* rec)
{
    if (uv_is_active((uv_handle_t*)&rec->addon.timer))
    {
        ev_list_erase(&scheduler->wait_queue, &rec->sc_node);
        ev_list_push_back(&scheduler->busy_queue, &rec->sc_node);
    }

    uv_timer_stop(&rec->addon.timer);
    uv_idle_start(&rec->addon.execution, _task_on_coroutine_run);
}

static int _task_find_and_set_as_ready(infra_scheduler_t* scheduler,
    lua_State* thread)
{
    coroutine_record_t tmp_key;
    tmp_key.data.co_thread = thread;

    ev_map_node_t* it = ev_map_find(&scheduler->co_map, &tmp_key.co_node);
    if (it == NULL)
    {
        return INFRA_ENOENT;
    }

    coroutine_record_t* rec = container_of(it, coroutine_record_t, co_node);
    _task_set_as_ready(scheduler, rec);

    return INFRA_SUCCESS;
}

static infra_scheduler_t* _task_get_instance(lua_State* L)
{
    infra_scheduler_t* scheduler;
    int obj_type = lua_getglobal(L, INFRA_SCHEDULER);

    /* Check global scheduler */
    if (obj_type == LUA_TUSERDATA)
    {
        scheduler = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return scheduler;
    }

    scheduler = lua_newuserdata(L, sizeof(infra_scheduler_t));
    assert(scheduler != NULL);

    ev_map_init(&scheduler->co_map, _task_on_cmp, NULL);
    ev_list_init(&scheduler->busy_queue);
    ev_list_init(&scheduler->wait_queue);
    ev_list_init(&scheduler->dead_queue);
    uv_loop_init(&scheduler->loop);
    scheduler->host_vm = NULL;
    scheduler->cnt_dead = 0;
    _task_set_metatable(L);

    lua_setglobal(L, INFRA_SCHEDULER);

    return scheduler;
}

static void _task_on_timeout(uv_timer_t *handle)
{
    coroutine_record_t* rec = container_of(handle, coroutine_record_t, addon.timer);
    infra_scheduler_t* scheduler = _task_get_instance(rec->data.co_thread);
    _task_find_and_set_as_ready(scheduler, rec->data.co_thread);
}

static void _task_set_as_wait(infra_scheduler_t* scheduler,
    coroutine_record_t* rec, uint64_t timeout)
{
    if (!uv_is_active((uv_handle_t*)&rec->addon.timer))
    {
        ev_list_erase(&scheduler->busy_queue, &rec->sc_node);
        ev_list_push_back(&scheduler->wait_queue, &rec->sc_node);
    }

    uv_idle_stop(&rec->addon.execution);
    uv_timer_start(&rec->addon.timer, _task_on_timeout, timeout, 0);
}

static int _task_find_and_set_as_wait(infra_scheduler_t* scheduler,
    lua_State* thread, uint64_t timeout)
{
    coroutine_record_t tmp_key;
    tmp_key.data.co_thread = thread;

    ev_map_node_t* it = ev_map_find(&scheduler->co_map, &tmp_key.co_node);
    if (it == NULL)
    {
        return INFRA_ENOENT;
    }

    coroutine_record_t* rec = container_of(it, coroutine_record_t, co_node);
    _task_set_as_wait(scheduler, rec, timeout);

    return INFRA_SUCCESS;
}

static void _task_init_coroutine(infra_scheduler_t* scheduler,
    coroutine_record_t* rec, lua_State* thread)
{
    rec->scheduler = scheduler;
    rec->data.co_thread = thread;
    rec->data.co_ref_key = LUA_NOREF;   // update later
    uv_idle_init(&scheduler->loop, &rec->addon.execution);
    uv_timer_init(&scheduler->loop, &rec->addon.timer);
    uv_idle_start(&rec->addon.execution, _task_on_coroutine_run);
}

int infra_task_run(lua_State* L)
{
    int ret;
    infra_scheduler_t* scheduler = _task_get_instance(L);
    assert(scheduler != NULL);

    scheduler->host_vm = L;
    ret = uv_run(&scheduler->loop, UV_RUN_DEFAULT);
    scheduler->host_vm = NULL;

    lua_pushinteger(L, ret);
    return 1;
}

int infra_task_spawn(lua_State* L)
{
    int ret = INFRA_SUCCESS;

    infra_scheduler_t* scheduler = _task_get_instance(L);
    assert(scheduler != NULL);

    lua_State* thread = lua_tothread(L, 1);
    assert(thread != NULL);

    coroutine_record_t* rec = malloc(sizeof(coroutine_record_t));
    assert(rec != NULL);
    _task_init_coroutine(scheduler, rec, thread);

    if (ev_map_insert(&scheduler->co_map, &rec->co_node) != 0)
    {/* Record already exist */
        ret = INFRA_EEXIST;
        free(rec);
        goto fin;
    }

    rec->data.co_ref_key = luaL_ref(L, LUA_REGISTRYINDEX);
    ev_list_push_back(&scheduler->busy_queue, &rec->sc_node);

fin:
    lua_pushinteger(L, ret);
    return 1;
}

int infra_task_ready(lua_State* L)
{
    infra_scheduler_t* scheduler = _task_get_instance(L);
    assert(scheduler != NULL);

    lua_State* thread = lua_tothread(L, 1);
    assert(thread != NULL);

    int ret = _task_find_and_set_as_ready(scheduler, thread);
    lua_pushinteger(L, ret);

    return 1;
}

int infra_task_wait(lua_State* L)
{
    infra_scheduler_t* scheduler = _task_get_instance(L);
    assert(scheduler != NULL);

    lua_State* thread = lua_tothread(L, 1);
    assert(thread != NULL);

    int is_number = 0;
    uint64_t timeout = lua_tointegerx(L, 2, &is_number);
    if (!is_number)
    {
        timeout = (uint64_t)-1;
    }

    int ret = _task_find_and_set_as_wait(scheduler, thread, timeout);
    lua_pushinteger(L, ret);

    return 1;
}

int infra_task_info(lua_State* L)
{
    infra_scheduler_t* scheduler = _task_get_instance(L);
    assert(scheduler != NULL);

    lua_newtable(L);

    lua_pushstring(L, "busy");
    lua_pushinteger(L, ev_list_size(&scheduler->busy_queue));
    lua_settable(L, -3);

    lua_pushstring(L, "wait");
    lua_pushinteger(L, ev_list_size(&scheduler->wait_queue));
    lua_settable(L, -3);

    lua_pushstring(L, "dead");
    lua_pushinteger(L, scheduler->cnt_dead);
    lua_settable(L, -3);

    return 1;
}
