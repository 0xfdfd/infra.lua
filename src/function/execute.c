#include "__init__.h"
#include <assert.h>
#include <malloc.h>
#include "utils/exec.h"
#include "utils/pipe.h"

#if defined(_WIN32)
#   include <wchar.h>
#else
#   include <unistd.h>
#endif

#define INFRA_EXECUTE_NAME  "__infra_execute"

typedef struct infra_execute
{
    char*           file;
    char*           cwd;

    /**
     * @brief Argument array.
     * The last element is always NULL, so the actual length is `arg_sz + 1`.
     */
    char**          args;
    size_t          arg_sz; /**< Array size of `arg` */

    /**
     * @brief Environment array.
     * The last element is always NULL, so the actual length is `env_sz + 1`.
     */
    char**          envs;
    size_t          env_sz;

    const char*     input;
    size_t          input_sz;
    int             input_ref;

    infra_os_pipe_t pip_stdin[2];
    infra_os_pipe_t pip_stdout[2];
    infra_os_pipe_t pip_stderr[2];

    infra_os_pid_t  hProcess;
} infra_execute_t;

static int _infra_execute_meta_gc(lua_State* L)
{
    size_t i;
    infra_execute_t* self = lua_touserdata(L, 1);

    if (self->file != NULL)
    {
        free(self->file);
        self->file = NULL;
    }

    if (self->cwd != NULL)
    {
        free(self->cwd);
        self->cwd = NULL;
    }

    if (self->args != NULL)
    {
        for (i = 0; self->args[i] != NULL; i++)
        {
            free(self->args[i]);
            self->args[i] = NULL;
        }
        free(self->args);
        self->args = NULL;
    }

    if (self->envs != NULL)
    {
        for (i = 0; self->envs[i] != NULL; i++)
        {
            free(self->envs[i]);
            self->envs[i] = NULL;
        }
        free(self->envs);
        self->envs = NULL;
    }

    if (self->input_ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, self->input_ref);
        self->input_ref = LUA_NOREF;
        self->input = NULL;
        self->input_sz = 0;
    }

    for (i = 0; i < ARRAY_SIZE(self->pip_stdin); i++)
    {
        if (self->pip_stdin[i] != INFRA_OS_PIPE_INVALID)
        {
            infra_pipe_close(self->pip_stdin[i]);
            self->pip_stdin[i] = INFRA_OS_PIPE_INVALID;
        }
        if (self->pip_stdout[i] != INFRA_OS_PIPE_INVALID)
        {
            infra_pipe_close(self->pip_stdout[i]);
            self->pip_stdout[i] = INFRA_OS_PIPE_INVALID;
        }
        if (self->pip_stderr[i] != INFRA_OS_PIPE_INVALID)
        {
            infra_pipe_close(self->pip_stderr[i]);
            self->pip_stderr[i] = INFRA_OS_PIPE_INVALID;
        }
    }

    if (self->hProcess != INFRA_OS_PID_INVALID)
    {
        infra_waitpid(self->hProcess, NULL, INFRA_OS_WAITPID_INFINITE);
        self->hProcess = INFRA_OS_PID_INVALID;
    }

    return 0;
}

static void _infra_execute_init_empty(infra_execute_t* self)
{
    memset(self, 0, sizeof(*self));
    self->input_ref = LUA_NOREF;

    size_t i;
    for (i = 0; i < ARRAY_SIZE(self->pip_stdin); i++)
    {
        self->pip_stdin[i] = INFRA_OS_PIPE_INVALID;
        self->pip_stdout[i] = INFRA_OS_PIPE_INVALID;
        self->pip_stderr[i] = INFRA_OS_PIPE_INVALID;
    }

    self->hProcess = INFRA_OS_PID_INVALID;
}

static int _infra_execute_init_self(lua_State* L, infra_execute_t* self, const char* file)
{
    if ((self->file = strdup(file)) == NULL)
    {
        goto error_oom;
    }

    self->arg_sz = 0;
    if ((self->args = malloc(sizeof(char*) * (self->arg_sz + 1))) == NULL)
    {
        goto error_oom;
    }
    self->args[0] = NULL;

    self->env_sz = 0;
    self->envs = NULL;

    return 0;

error_oom:
    return luaL_error(L, "out of memory.");
}

static int _infra_execute_init_args_field(lua_State* L, int idx, char* name, char*** field, size_t* field_sz)
{
    int i;
    int sp = lua_gettop(L);

    if (lua_getfield(L, idx, name) != LUA_TNIL)
    {
        for (i = 1; lua_geti(L, sp + 1, i) == LUA_TSTRING; i++)
        {
            size_t new_sz = *field_sz + 1;
            char** new_arg = realloc(*field, sizeof(char*) * (new_sz + 1));
            if (new_arg == NULL)
            {
                goto error_oom;
            }
            new_arg[new_sz] = NULL;
            *field = new_arg;
            *field_sz = new_sz;

            if ((new_arg[new_sz - 1] = strdup(lua_tostring(L, -1))) == NULL)
            {
                goto error_oom;
            }

            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return 0;

error_oom:
    return luaL_error(L, "out of memory.");
}

static int _infra_execute_init_args(lua_State* L, infra_execute_t* self, int idx)
{
    _infra_execute_init_args_field(L, idx, "args", &self->args, &self->arg_sz);
    _infra_execute_init_args_field(L, idx, "envs", &self->envs, &self->env_sz);

    if (lua_getfield(L, idx, "cwd") == LUA_TSTRING)
    {
        if ((self->cwd = strdup(lua_tostring(L, -1))) == NULL)
        {
            goto error_oom;
        }
    }
    lua_pop(L, 1);

    if (lua_getfield(L, idx, "stdin") == LUA_TSTRING)
    {
        lua_pushvalue(L, -1);
        self->input = lua_tolstring(L, -1, &self->input_sz);
        self->input_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);

    return 0;

error_oom:
    return luaL_error(L, "out of memory.");
}

static int _infra_execute_open_pip_stdio(lua_State* L, infra_execute_t* self)
{
    int errcode = 0;

    if ((errcode = infra_pipe_open(self->pip_stdin, 0, INFRA_OS_PIPE_NONBLOCK | INFRA_OS_PIPE_CLOEXEC)) != 0)
    {
        goto error_open_stdio;
    }

    if ((errcode = infra_pipe_open(self->pip_stdout, INFRA_OS_PIPE_NONBLOCK | INFRA_OS_PIPE_CLOEXEC, 0)) != 0)
    {
        goto error_open_stdio;
    }

    if ((errcode = infra_pipe_open(self->pip_stderr, INFRA_OS_PIPE_NONBLOCK | INFRA_OS_PIPE_CLOEXEC, 0)) != 0)
    {
        goto error_open_stdio;
    }

    return 0;

error_open_stdio:
    return infra_raise_error(L, errcode);
}

static int _infra_execute_exec(lua_State* L, infra_execute_t* self)
{
    int ret;
    
    infra_exec_opt_t opt = {
        self->cwd,
        self->args,
        self->envs,
        self->pip_stdin[0],
        self->pip_stdout[1],
        self->pip_stderr[1],
    };
    if ((ret = infra_exec(&self->hProcess, self->file, &opt)) != 0)
    {
        return infra_raise_error(L, ret);
    }

    /* Close unused things. */
    infra_pipe_close(self->pip_stdin[0]); self->pip_stdin[0] = INFRA_OS_PIPE_INVALID;
    infra_pipe_close(self->pip_stdout[1]); self->pip_stdout[1] = INFRA_OS_PIPE_INVALID;
    infra_pipe_close(self->pip_stderr[1]); self->pip_stderr[1] = INFRA_OS_PIPE_INVALID;

    return 0;
}

static int _infra_execute_exchange_handle_stdin(infra_execute_t* self)
{
    while (self->input_sz > 0 && self->pip_stdin[1] != INFRA_OS_PIPE_INVALID)
    {
        ssize_t write_sz = infra_pipe_write(self->pip_stdin[1], self->input, self->input_sz);
        if (write_sz < 0)
        {
            return (int)write_sz;
        }
        else if (write_sz == 0)
        {
            break;
        }

        self->input_sz -= write_sz;
        self->input += write_sz;
    }

    if (self->input_sz == 0 && self->pip_stdin[1] != INFRA_OS_PIPE_INVALID)
    {
        infra_pipe_close(self->pip_stdin[1]);
        self->pip_stdin[1] = INFRA_OS_PIPE_INVALID;
    }

    return 0;
}

static int _infra_execute_handle_stdout(lua_State* L, infra_execute_t* self, void* buf, size_t buf_sz, int idx_stdout)
{
    ssize_t ret;
    while (self->pip_stdout[0] != INFRA_OS_PIPE_INVALID)
    {
        ret = infra_pipe_read(self->pip_stdout[0], buf, buf_sz);
        if (ret > 0)
        {
            lua_pushvalue(L, idx_stdout);
            lua_pushlstring(L, buf, ret);
            lua_concat(L, 2);
            lua_replace(L, idx_stdout);
        }
        /* No data received, try again. */
        else if (ret == 0)
        {
            break;
        }
        /* Read failed, Get errno and check pipe status */
        else if (ret == INFRA_EOF)
        {
            infra_pipe_close(self->pip_stdout[0]);
            self->pip_stdout[0] = INFRA_OS_PIPE_INVALID;
        }
        else
        {
            return (int)ret;
        }
    }

    return 0;
}

static int _infra_execute_handle_stderr(lua_State* L, infra_execute_t* self, void* buf, size_t buf_sz, int idx_stderr)
{
    ssize_t ret;
    while (self->pip_stderr[0] != INFRA_OS_PIPE_INVALID)
    {
        ret = infra_pipe_read(self->pip_stderr[0], buf, buf_sz);
        if (ret > 0)
        {
            lua_pushvalue(L, idx_stderr);
            lua_pushlstring(L, buf, ret);
            lua_concat(L, 2);
            lua_replace(L, idx_stderr);
        }
        /* No data received, try again. */
        else if (ret == 0)
        {
            break;
        }
        /* Read failed, Get errno and check pipe status */
        else if (ret == INFRA_EOF)
        {
            infra_pipe_close(self->pip_stderr[0]);
            self->pip_stderr[0] = INFRA_OS_PIPE_INVALID;
        }
        else
        {
            return (int)ret;
        }
    }

    return 0;
}

static int _infra_execute_exchange(lua_State* L, infra_execute_t* self, int idx_ret, int idx_stdout, int idx_stderr)
{
    int ret, errcode;
    idx_ret = lua_absindex(L, idx_ret);
    idx_stdout = lua_absindex(L, idx_stdout);
    idx_stderr = lua_absindex(L, idx_stderr);

    int sp = lua_gettop(L);
    const size_t buf_sz = 4096;
    char* buf = infra_tmpbuf(L, buf_sz);

    int exit_code;
    while ((ret = infra_waitpid(self->hProcess, &exit_code, 10)) == INFRA_ETIMEDOUT)
    {
        if ((errcode = _infra_execute_exchange_handle_stdin(self)) != 0)
        {
            goto io_error;
        }

        /* Handle stdout. */
        if ((errcode = _infra_execute_handle_stdout(L, self, buf, buf_sz, idx_stdout)) != 0)
        {
            goto io_error;
        }

        /* Handle stderr. */
        if ((errcode = _infra_execute_handle_stderr(L, self, buf, buf_sz, idx_stderr)) != 0)
        {
            goto io_error;
        }
    }

    assert(ret == 0);
    self->hProcess = INFRA_OS_PID_INVALID;

    lua_pushboolean(L, !exit_code);
    lua_replace(L, idx_ret);

    /* Handle stdout and stderr again. */
    if ((errcode = _infra_execute_handle_stdout(L, self, buf, buf_sz, idx_stdout)) != 0)
    {
        goto io_error;
    }
    if ((errcode = _infra_execute_handle_stderr(L, self, buf, buf_sz, idx_stderr)) != 0)
    {
        goto io_error;
    }

    lua_settop(L, sp);
    return 0;

io_error:
    lua_settop(L, sp);
    infra_push_error(L, errcode);
    return -1;
}

static int _infra_execute(lua_State* L)
{
    //int sp = lua_gettop(L);
    const char* file = luaL_checkstring(L, 1);

    infra_execute_t* self = lua_newuserdata(L, sizeof(infra_execute_t)); // sp+1
    _infra_execute_init_empty(self);

    static const luaL_Reg s_meta[] = {
        { "__gc",   _infra_execute_meta_gc },
        { NULL,     NULL },
    };
    if (luaL_newmetatable(L, INFRA_EXECUTE_NAME) != 0)
    {
        luaL_setfuncs(L, s_meta, 0);
    }
    lua_setmetatable(L, -2);

    _infra_execute_init_self(L, self, file);
    _infra_execute_init_args(L, self, 2);

    /* Platform related operations. */
    _infra_execute_open_pip_stdio(L, self);
    _infra_execute_exec(L, self);

    /* Prepare return value. */
    lua_pushboolean(L, 1); // sp+2
    lua_pushstring(L, ""); // sp+3
    lua_pushstring(L, ""); // sp+4

    if (_infra_execute_exchange(L, self, -3, -2, -1) < 0)
    {
        return lua_error(L);
    }

    return 3;
}

const infra_lua_api_t infra_f_execute = {
"execute", _infra_execute, 0,
"Execute program.",

"[SYNOPSIS]\n"
"boolean,string,string execute(string file [, table opt])\n"
};
