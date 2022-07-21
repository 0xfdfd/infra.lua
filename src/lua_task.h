#ifndef __INFRA_API_TASK_H__
#define __INFRA_API_TASK_H__

#include "lua_api.h"

#define INFRA_LUA_API_TASK_RUN(XX)                                                      \
XX(                                                                                     \
"task_run", infra_task_run, NULL,                                                       \
"Schedule all registered coroutines until all coroutine is dead.",                      \
"SYNOPSIS"                                                                          "\n"\
"    integer task_run();"                                                           "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    Run a scheduler until all coroutine finished or was interrupted."              "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    On success, the `task_run()` return 0. If the scheduler was interrupted,"      "\n"\
"    the number of registered thread is returned."                                  "\n"\
)

#define INFRA_LUA_API_TASK_SPAWN(XX)                                                    \
XX(                                                                                     \
"task_spawn", infra_task_spawn, NULL,                                                   \
"Accepts a thread and resumes it as soon as possible.",                                 \
"SYNOPSIS"                                                                          "\n"\
"    errno task_spawn(coroutine, ...);"                                             "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `task_spawn()` function take a thread and put it into end of"              "\n"\
"    scheduling queue."                                                             "\n"\
                                                                                    "\n"\
"    The coroutine will not be scheduled until somebody set it as ready with"       "\n"\
"    `scheduler_ready()`."                                                          "\n"\
                                                                                    "\n"\
"    You can not register the same coroutine twice."                                "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    On success, `task_spawn()` return 0. On error, a negative error code is"       "\n"\
"    returned."                                                                     "\n"\
)

#define INFRA_LUA_API_TASK_READY(XX)                                                    \
XX(                                                                                     \
"task_ready", infra_task_ready, NULL,                                                   \
"Set coroutine as ready to be scheduled",                                               \
"SYNOPSIS"                                                                          "\n"\
"    integer task_ready(coroutine);"                                                "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `task_ready()` function set a coroutine as READY, so scheduler is"         "\n"\
"    allowed to schedule it."                                                       "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    On success, `task_ready()` return 0. If failed, an error code is returned."    "\n"\
)

#define INFRA_LUA_API_TASK_WAIT(XX)                                                     \
XX(                                                                                     \
"task_wait", infra_task_wait, NULL,                                                     \
"Set coroutine as wait infinite.",                                                      \
"SYNOPSIS"                                                                          "\n"\
"    integer task_wait(coroutine, timeout);"                                        "\n"\
                                                                                    "\n"\
"DESCRIPTION"                                                                       "\n"\
"    The `task_wait()` function set a coroutine as WAIT. This function does not"    "\n"\
"    stop that coroutine."                                                          "\n"\
                                                                                    "\n"\
"    The `coroutine` parameter must a registered coroutine by `task_spawn()`."      "\n"\
                                                                                    "\n"\
"    The optional `timeout` parameter is an integer, specify how much time in"      "\n"\
"    milliseconds the coroutine become READY again. If empty or a negative"         "\n"\
"    value, the coroutine will wait forever."                                       "\n"\
                                                                                    "\n"\
"    A coroutine in WAIT status will not be scheduled unless:"                      "\n"\
"    1. Use `task_ready()` to set it as READY."                                     "\n"\
"    2. The `timeout` parameter expired."                                           "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    On success, `task_wait()` return 0. If failed, an error code is returned."     "\n"\
)

#define INFRA_LUA_API_TASK_INFO(XX)                                                     \
XX(                                                                                     \
"task_info", infra_task_info, NULL,                                                     \
"Get information about registered coroutines",                                          \
"SYNOPSIS"                                                                          "\n"\
"    table task_info();"                                                            "\n"\
                                                                                    "\n"\
"RETURN VALUE"                                                                      "\n"\
"    The `task_info()` function return a table for schedule information."           "\n"\
                                                                                    "\n"\
"    The table have following structure:"                                           "\n"\
"    ```lua"                                                                        "\n"\
"    {"                                                                             "\n"\
"        [busy] = integer,"                                                         "\n"\
"        [wait] = integer,"                                                         "\n"\
"        [dead] = integer,"                                                         "\n"\
"    }"                                                                             "\n"\
"    ```"                                                                           "\n"\
"    Each field have following means:"                                              "\n"\
"    + `busy`: The number of threads currently running."                            "\n"\
"    + `wait`: The number of threads wait to schedule (that called"                 "\n"\
"      `task_wait()`)"                                                              "\n"\
"    + `dead`: The number of threads that already dead."                            "\n"\
)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see INFRA_LUA_API_TASK_RUN
 */
API_LOCAL int infra_task_run(lua_State* L);

/**
 * @see INFRA_LUA_API_TASK_SPAWN
 */
API_LOCAL int infra_task_spawn(lua_State* L);

/**
 * @see INFRA_LUA_API_TASK_READY
 */
API_LOCAL int infra_task_ready(lua_State* L);

/**
 * @see INFRA_LUA_API_TASK_WAIT
 */
API_LOCAL int infra_task_wait(lua_State* L);

/**
 * @see INFRA_LUA_API_TASK_INFO
 */
API_LOCAL int infra_task_info(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
