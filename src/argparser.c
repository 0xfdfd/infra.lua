#include "__init__.h"
#include "utils/list.h"
#include "utils/map.h"
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>

#define INFRA_ARGPARSER_NAME    "__infra_argparser"

typedef struct infra_argp infra_argp_t;

typedef struct infra_argp_opt
{
    ev_list_node_t          node;
    infra_argp_t*           belong;

    char**                  names;
    size_t                  name_sz;

    /*
     * The following is the attributes of this argument. The prefix is the type
     * of the field:
     * + `f_*`: This is a Lua function.
     * + `r_*`: This is a Lua value.
     * + `c_*`: This is a C value.
     */
    int                     f_action;   /**< (REF) The basic type of action to be taken when this argument is encountered at the command line. */
    int                     f_type;     /**< (REF) The type to which the command-line argument should be converted. */
    int                     v_choices;  /**< (REF) Allowable values for the argument. */
    int                     v_default;  /**< (REF) The value produced if the argument is absent. */
    char*                   c_type;     /**< The type name of #f_type. */
    char*                   c_help;     /**< A brief description of what the argument does. */
    int                     c_required; /**< Whether an argument is required or optional. */
    int                     c_narg;     /**< N / -'?' / -'*' / -'+'. */
} infra_argp_opt_t;

struct infra_argp
{
    char*                   prog;
    char*                   desc;
    char*                   epilog;

    ev_list_t               arg_table;  /**< Save #infra_argp_opt_t. */
};

typedef struct infra_argp_helper
{
    lua_Integer             pos_arg;    /**< Position of current argument in array. */
    infra_argp_t*           self;
    infra_argp_opt_t*       opt;        /**< The current option processing. */
    int                     narg;       /**< The number of arguments to be save. */
    int                     idx_argp;   /**< Index of argparser. */
    int                     idx_ret;    /**< Index of return value. */
} infra_argp_helper_t;

static void _infra_argp_clear_option_field_action(lua_State* L, infra_argp_opt_t* arg)
{
    if (arg->f_action != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, arg->f_action);
        arg->f_action = LUA_NOREF;
    }
}

static void _infra_argp_clear_option_field_help(infra_argp_opt_t* arg)
{
    if (arg->c_help != NULL)
    {
        free(arg->c_help);
        arg->c_help = NULL;
    }
}

static void _infra_argp_clear_option_field_c_type(infra_argp_opt_t* opt)
{
    if (opt->c_type != NULL)
    {
        free(opt->c_type);
        opt->c_type = NULL;
    }
}

static void _infra_argp_clear_option_field_f_type(lua_State* L, infra_argp_opt_t* arg)
{
    if (arg->f_type != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, arg->f_type);
        arg->f_type = LUA_NOREF;
    }
}

static void _infra_argp_clear_arg_table(lua_State* L, infra_argp_t* self)
{
    ev_list_node_t* it;
    while ((it = ev_list_pop_front(&self->arg_table)) != NULL)
    {
        infra_argp_opt_t* arg = container_of(it, infra_argp_opt_t, node);

        size_t i;
        for (i = 0; i < arg->name_sz; i++)
        {
            free(arg->names[i]);
            arg->names[i] = NULL;
        }
        free(arg->names);
        arg->names = NULL;
        arg->name_sz = 0;

        _infra_argp_clear_option_field_help(arg);
        _infra_argp_clear_option_field_action(L, arg);
        _infra_argp_clear_option_field_c_type(arg);
        _infra_argp_clear_option_field_f_type(L, arg);

        if (arg->v_choices != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, arg->v_choices);
            arg->v_choices = LUA_NOREF;
        }
        if (arg->v_default != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, arg->v_default);
            arg->v_default = LUA_NOREF;
        }

        free(arg);
        arg = NULL;
    }
}

static int _infra_argp_meta_gc(lua_State* L)
{
    infra_argp_t* self = lua_touserdata(L, 1);

    if (self->prog != NULL)
    {
        free(self->prog);
        self->prog = NULL;
    }

    if (self->desc != NULL)
    {
        free(self->desc);
        self->desc = NULL;
    }

    if (self->epilog != NULL)
    {
        free(self->epilog);
        self->epilog = NULL;
    }

    _infra_argp_clear_arg_table(L, self);

    return 0;
}

static infra_argp_opt_t* _infra_argp_find_option(lua_State* L, infra_argp_t* self, const char* name)
{
    ev_list_node_t* it = ev_list_begin(&self->arg_table);
    for (; it != NULL; it = ev_list_next(it))
    {
        infra_argp_opt_t* arg = container_of(it, infra_argp_opt_t, node);

        size_t i;
        for (i = 0; i < arg->name_sz; i++)
        {
            if (strcmp(arg->names[i], name) == 0)
            {
                return arg;
            }
        }
    }

    luaL_error(L, "%s: error: cannot find option name `%s`.",
        self->prog, name);
    return NULL;
}

static int _infra_argp_action_store(lua_State* L)
{
    lua_pushvalue(L, 3);
    return 1;
}

static int _infra_argp_action_append(lua_State* L)
{
    if (lua_type(L, 2) != LUA_TTABLE)
    {
        lua_newtable(L);
    }
    else
    {
        lua_pushvalue(L, 2);
    }

    lua_pushvalue(L, 3);
    lua_seti(L, -2, luaL_len(L, -2) + 1);

    return 1;
}

static int _infra_argp_action_count(lua_State* L)
{
    /* Update counter. */
    if (lua_type(L, 2) != LUA_TNUMBER)
    {
        lua_pushinteger(L, 1);
    }
    else
    {
        lua_Integer n = lua_tointeger(L, 2);
        lua_pushinteger(L, n + 1);
    }

    /* Return updated value. */
    return 1;
}

static int _infra_argp_gen_help_usage(lua_State* L, infra_argp_t* self, int idx_ret)
{
    lua_pushvalue(L, idx_ret);

    lua_pushfstring(L, "usage: %s", self->prog);
    lua_concat(L, 2);

    ev_list_node_t* it = ev_list_begin(&self->arg_table);
    for (; it != NULL; it = ev_list_next(it))
    {
        infra_argp_opt_t* opt = container_of(it, infra_argp_opt_t, node);
        lua_pushfstring(L, " [%s]", opt->names[0]);
        lua_concat(L, 2);
    }

    lua_pushstring(L, "\n");
    lua_concat(L, 2);

    lua_replace(L, idx_ret);
    return 0;
}

static int _infra_argp_gen_help_description(lua_State* L, infra_argp_t* self, int idx_ret)
{
    if (self->desc != NULL)
    {
        lua_pushvalue(L, idx_ret);
        lua_pushfstring(L, "%s\n", self->desc);
        lua_concat(L, 2);
        lua_replace(L, idx_ret);
    }

    return 0;
}

static int _infra_argp_merge_string_array(lua_State* L)
{
    int sp = lua_gettop(L);

    lua_pushstring(L, "(");

    int i;
    for (i = 1; i <= sp; i++)
    {
        if (i != 1)
        {
            lua_pushstring(L, ", ");
            lua_concat(L, 2);
        }

        lua_pushvalue(L, i);
        lua_concat(L, 2);
    }

    lua_pushstring(L, ")");
    lua_concat(L, 2);

    return 1;
}

static int _infra_argp_gen_notes(lua_State* L)
{
    int n_args = 0;
    infra_argp_opt_t* opt = lua_touserdata(L, 1);

    lua_pushstring(L, "  ");
    lua_pushcfunction(L, _infra_argp_merge_string_array);

    if (opt->c_narg != 0 && opt->c_type != NULL)
    {
        lua_pushstring(L, opt->c_type);
        n_args++;
    }

    if (opt->c_required)
    {
        lua_pushstring(L, "required");
        n_args++;
    }

    if (opt->v_default != LUA_NOREF)
    {
        lua_pushfstring(L, "default: ");
        lua_rawgeti(L, LUA_REGISTRYINDEX, opt->v_default);
        lua_concat(L, 2);
        n_args++;
    }

    if (n_args == 0)
    {
        return 0;
    }

    lua_call(L, n_args, 1);
    lua_concat(L, 2);

    return 1;
}

static int _infra_argp_gen_help_option(lua_State* L,
    infra_argp_t* self, int idx_ret, int is_positional)
{
    lua_pushvalue(L, idx_ret);

    size_t cnt;
    ev_list_node_t* it = ev_list_begin(&self->arg_table);
    for (cnt = 0; it != NULL; it = ev_list_next(it))
    {
        infra_argp_opt_t* arg = container_of(it, infra_argp_opt_t, node);
        if ((is_positional && arg->names[0][0] == '-')
            || (!is_positional && arg->names[0][0] != '-'))
        {
            continue;
        }

        if (cnt == 0)
        {
            if (is_positional)
            {
                lua_pushstring(L, "\npositional arguments:\n");
            }
            else
            {
                lua_pushstring(L, "\noptions:\n");
            }
            lua_concat(L, 2);
        }
        cnt++;

        size_t i;
        for (i = 0; i < arg->name_sz; i++)
        {
            if (i == 0)
            {
                lua_pushfstring(L, " %s", arg->names[i]);
            }
            else
            {
                lua_pushfstring(L, ", %s", arg->names[i]);
            }
            lua_concat(L, 2);
        }
        lua_pushstring(L, "\n");
        lua_concat(L, 2);

        if (arg->c_help != NULL)
        {
            lua_pushfstring(L, "  %s\n", arg->c_help);
            lua_concat(L, 2);
        }

        lua_pushcfunction(L, _infra_argp_gen_notes);
        lua_pushlightuserdata(L, arg);
        lua_call(L, 1, 1);
        if (lua_type(L, -1) == LUA_TSTRING)
        {
            lua_pushstring(L, "\n");
            lua_concat(L, 3);
        }
        else
        {
            lua_pop(L, 1);
        }
    }

    lua_replace(L, idx_ret);
    return 0;
}

static int _infra_argp_gen_help_positional_arguments(lua_State* L,
    infra_argp_t* self, int idx_ret)
{
    return _infra_argp_gen_help_option(L, self, idx_ret, 1);
}

static int _infra_argp_gen_help_options(lua_State* L, infra_argp_t* self, int idx_ret)
{
    return _infra_argp_gen_help_option(L, self, idx_ret, 0);
}

static int _infra_argp_gen_help_epilog(lua_State* L, infra_argp_t* self, int idx_ret)
{
    if (self->epilog == NULL)
    {
        return 0;
    }

    lua_pushvalue(L, idx_ret);
    lua_pushfstring(L, "\n%s\n", self->epilog);
    lua_concat(L, 2);

    lua_replace(L, idx_ret);
    return 0;
}

static int _infra_argp_format_help(lua_State* L)
{
    infra_argp_t* self = luaL_checkudata(L, 1, INFRA_ARGPARSER_NAME);
    int sp = lua_gettop(L);

    lua_pushstring(L, ""); // sp+1

    _infra_argp_gen_help_usage(L, self, sp + 1);
    _infra_argp_gen_help_description(L, self, sp + 1);
    _infra_argp_gen_help_positional_arguments(L, self, sp + 1);
    _infra_argp_gen_help_options(L, self, sp + 1);
    _infra_argp_gen_help_epilog(L, self, sp + 1);

    return 1;
}

static int _infra_argp_action_help(lua_State* L)
{
    lua_pushcfunction(L, _infra_argp_format_help);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    printf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    exit(EXIT_SUCCESS);
}

static int _infra_argp_get_action(lua_State* L, const char* action)
{
    static luaL_Reg s_action_map[] = {
        { "store",  _infra_argp_action_store },
        { "append", _infra_argp_action_append },
        { "count",  _infra_argp_action_count },
        { "help",   _infra_argp_action_help },
        { NULL,     NULL },
    };

    size_t i;
    for (i = 0; s_action_map[i].name != NULL; i++)
    {
        if (strcmp(s_action_map[i].name, action) == 0)
        {
            lua_pushcclosure(L, s_action_map[i].func, 0);
            return luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }

    return luaL_error(L, "unknown action `%s`.", action);
}

static int _infra_argp_type_string(lua_State* L)
{
    lua_pushvalue(L, 1);
    return 1;
}

static int _infra_argp_type_float(lua_State* L)
{
    const char* s = luaL_checkstring(L, 1);

    lua_pushvalue(L, 1);

    int isnum = 0;
    lua_Number n = lua_tonumberx(L, -1, &isnum);
    if (!isnum)
    {
        return luaL_error(L, "cannot convert `%s` to number.", s);
    }

    lua_pushnumber(L, n);
    return 1;
}

static int _infra_argp_get_type(lua_State* L, const char* type)
{
    static luaL_Reg s_type_map[] = {
        { "string", _infra_argp_type_string },
        { "number", _infra_argp_type_float },
        { NULL,     NULL },
    };

    size_t i;
    for (i = 0; s_type_map[i].name != NULL; i++)
    {
        if (strcmp(s_type_map[i].name, type) == 0)
        {
            lua_pushcfunction(L, s_type_map[i].func);
            return luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }

    return luaL_error(L, "unknown type `%s`.", type);
}

static int _infra_argp_add_argument_fill_with_default(lua_State* L,
    infra_argp_opt_t* arg)
{
    if (arg->f_action == LUA_NOREF)
    {
        arg->f_action = _infra_argp_get_action(L, "store");
    }

    if (arg->f_type == LUA_NOREF)
    {
        arg->c_type = strdup("string");
        INFRA_CHECK_OOM(L, arg->c_type);
        arg->f_type = _infra_argp_get_type(L, "string");
    }
    
    return 0;
}

static int _infra_argp_add_argument_action(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    int ret;

    if ((ret = lua_getfield(L, idx, "action")) == LUA_TNIL)
    {
        // do nothing
    }
    else if (ret == LUA_TSTRING)
    {
        const char* v = lua_tostring(L, -1);

        _infra_argp_clear_option_field_action(L, arg);
        arg->f_action = _infra_argp_get_action(L, v);

        if (strcmp(v, "help") == 0 && arg->c_help == NULL)
        {
            _infra_argp_clear_option_field_help(arg);
            arg->c_help = strdup("Show this help message and exit.");
            INFRA_CHECK_OOM(L, arg->c_help);
        }
    }
    else if (ret == LUA_TFUNCTION)
    {
        _infra_argp_clear_option_field_action(L, arg);

        lua_pushvalue(L, -1);
        arg->f_action = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        return luaL_error(L, "unknown option for `action`.");
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argp_add_argument_type(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    int ret;

    if ((ret = lua_getfield(L, idx, "type")) == LUA_TNIL)
    {
        // do nothing
    }
    else if (ret == LUA_TSTRING)
    {
        arg->c_type = strdup(lua_tostring(L, -1));
        INFRA_CHECK_OOM(L, arg->c_type);

        arg->f_type = _infra_argp_get_type(L, arg->c_type);
    }
    else if (ret == LUA_TFUNCTION)
    {
        arg->c_type = strdup("function");
        INFRA_CHECK_OOM(L, arg->c_type);

        lua_pushvalue(L, -1);
        arg->f_type = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        return luaL_error(L, "unknown option for `type`.");
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argp_add_argument_choices(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    int ret;
    if ((ret = lua_getfield(L, idx, "choices")) == LUA_TNIL)
    {
        // do nothing
    }
    else if (ret == LUA_TTABLE)
    {
        lua_pushvalue(L, -1);
        arg->v_choices = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        /* Check for meta-table field `__index`. */
        if ((ret = luaL_getmetafield(L, -1, "__index")) == LUA_TNIL)
        {
            return luaL_error(L, "unknown value for `choices`.");
        }
        lua_pop(L, 1);

        lua_pushvalue(L, -1);
        arg->v_choices = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argp_add_argument_default(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    int ret;

    if ((ret = lua_getfield(L, idx, "default")) != LUA_TNIL)
    {
        lua_pushvalue(L, -1);
        arg->v_default = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argp_add_argument_required(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    if (lua_getfield(L, idx, "required") != LUA_TNIL)
    {
        arg->c_required = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argp_add_argument_help(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    if (lua_getfield(L, idx, "help") == LUA_TSTRING)
    {
        _infra_argp_clear_option_field_help(arg);
        arg->c_help = strdup(lua_tostring(L, -1));
        INFRA_CHECK_OOM(L, arg->c_help);
    }
    lua_pop(L, 1);
    return 0;
}

static int _infra_argp_add_argument_narg(lua_State* L, infra_argp_opt_t* arg, int idx)
{
    int ret;
    if ((ret = lua_getfield(L, idx, "nargs")) == LUA_TNUMBER)
    {
        arg->c_narg = (int)lua_tointeger(L, -1);
        if (arg->c_narg < 0)
        {
            return luaL_error(L, "the value of `narg` must larger than 0.");
        }
    }
    else if (ret == LUA_TSTRING)
    {
        const char* v = lua_tostring(L, -1);
        switch (*v)
        {
        case '?':
            arg->c_narg = -'?';
            break;

        case '*':
            arg->c_narg = -'*';
            break;

        case '+':
            arg->c_narg = -'+';
            break;

        default:
            return luaL_error(L, "unknown value `%s` of `narg`.", v);
        }
    }
    lua_pop(L, 1);

    return 0;
}

static int _infra_argparser_add_argument_define(lua_State* L,
    infra_argp_opt_t* arg, int idx_opt)
{
    int ret;
    if ((ret = lua_type(L, idx_opt)) == LUA_TNIL)
    {
        goto finish;
    }

    _infra_argp_add_argument_action(L, arg, idx_opt);
    _infra_argp_add_argument_type(L, arg, idx_opt);
    _infra_argp_add_argument_choices(L, arg, idx_opt);
    _infra_argp_add_argument_default(L, arg, idx_opt);
    _infra_argp_add_argument_required(L, arg, idx_opt);
    _infra_argp_add_argument_narg(L, arg, idx_opt);
    _infra_argp_add_argument_help(L, arg, idx_opt);

finish:
    return _infra_argp_add_argument_fill_with_default(L, arg);
}

static int _infra_argp_add_argument(lua_State* L)
{
    infra_argp_t* self = luaL_checkudata(L, 1, INFRA_ARGPARSER_NAME);
    const char* opt_name = luaL_checkstring(L, 2);

    /* Make a copy. */
    size_t opt_name_sz = strlen(opt_name) + 1;
    char* copy_opt_name = infra_tmpbuf(L, opt_name_sz); // sp+1
    memcpy(copy_opt_name, opt_name, opt_name_sz);

    infra_argp_opt_t* new_arg = malloc(sizeof(infra_argp_opt_t));
    if (new_arg == NULL)
    {
        goto error_oom;
    }
    memset(new_arg, 0, sizeof(*new_arg));
    new_arg->belong = self;
    new_arg->f_action = LUA_NOREF;
    new_arg->f_type = LUA_NOREF;
    new_arg->v_choices = LUA_NOREF;
    new_arg->v_default = LUA_NOREF;
    ev_list_push_back(&self->arg_table, &new_arg->node);

    char* saveptr = NULL;
    char* tmp = NULL;
    while ((tmp = strtok_r(copy_opt_name, " ,", &saveptr)) != NULL)
    {
        copy_opt_name = NULL;
        size_t new_size = new_arg->name_sz + 1;
        char** new_names = realloc(new_arg->names, sizeof(char*) * new_size);
        if (new_names == NULL)
        {
            goto error_oom;
        }
        new_arg->name_sz = new_size;
        new_arg->names = new_names;

        if ((new_arg->names[new_arg->name_sz - 1] = strdup(tmp)) == NULL)
        {
            goto error_oom;
        }
    }

    lua_pop(L, 1);
    return _infra_argparser_add_argument_define(L, new_arg, 3);

error_oom:
    return luaL_error(L, INFRA_LUA_ERRMSG_OOM);
}

static int _infra_argp_check_args_valid(lua_State* L, infra_argp_t* self, int idx_ret)
{
    ev_list_node_t* it = ev_list_begin(&self->arg_table);
    for (; it != NULL; it = ev_list_next(it))
    {
        infra_argp_opt_t* arg = container_of(it, infra_argp_opt_t, node);

        int type = lua_getfield(L, idx_ret, arg->names[0]);
        lua_pop(L, 1);

        if (arg->c_required && type == LUA_TNIL)
        {
            return luaL_error(L, "%s: the following arguments are required: %s.",
                self->prog, arg->names[0]);
        }

        if (arg->c_narg == -'+' && type == LUA_TNIL)
        {
            return luaL_error(L, "%s: error: the following arguments are required: %s.",
                self->prog, arg->names[0]);
        }

        if (!arg->c_required && type == LUA_TNIL)
        {
            size_t i;
            for (i = 0; i < arg->name_sz; i++)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, arg->v_default);
                lua_setfield(L, idx_ret, arg->names[i]);
            }
        }
    }

    return 0;
}

static int _infra_argp_handle_option(lua_State* L, infra_argp_opt_t* opt,
    int idx_argp, int idx_ret, int idx_val)
{
    int sp = lua_gettop(L);
    idx_ret = lua_absindex(L, idx_ret);
    idx_val = lua_absindex(L, idx_val);

    /* Step 1. Type conversion. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, opt->f_type);
    lua_pushvalue(L, idx_val);
    lua_call(L, 1, 1); // sp+1

    /* Step 2. Check for valid. */
    if (opt->v_choices != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, opt->v_choices); // sp+2
        lua_pushvalue(L, sp + 1); // sp+3
        
        if (lua_gettable(L, sp + 2) != LUA_TNIL) // sp+3
        {
            if (lua_toboolean(L, -1) == 0)
            {
                return luaL_error(L, "%s: error: value `%s` for option `%s` does not match choices.",
                    opt->belong->prog, lua_tostring(L, idx_val), opt->names[0]);
            }
        }

        lua_pop(L, 2);
    }

    /* Step 2. Generate captured value. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, opt->f_action);
    lua_pushvalue(L, idx_argp);
    lua_getfield(L, idx_ret, opt->names[0]);
    lua_pushvalue(L, sp + 1);
    lua_call(L, 3, 1); // sp+2

    /* Update storage. */
    size_t i;
    for (i = 0; i < opt->name_sz; i++)
    {
        lua_pushvalue(L, sp + 2);
        lua_setfield(L, idx_ret, opt->names[i]);
    }

    /* Reset stack. */
    lua_settop(L, sp);

    return 0;
}

static int _infra_argp_parse_args_remain(lua_State* L, infra_argp_helper_t* helper, int idx_val)
{
    if (helper->narg > 0)
    {
        _infra_argp_handle_option(L, helper->opt, helper->idx_argp, helper->idx_ret, idx_val);
        helper->narg--;
        if (helper->narg == 0)
        {
            helper->opt = NULL;
        }
    }
    else if (helper->narg == 0)
    {
        helper->opt = NULL;
        abort(); // should not happen.
    }
    else if (helper->narg == -'?')
    {
        _infra_argp_handle_option(L, helper->opt, helper->idx_argp, helper->idx_ret, idx_val);
        helper->narg = 0;
        helper->opt = NULL;
    }
    else if (helper->narg == -'*' || helper->narg == -'+')
    {
        _infra_argp_handle_option(L, helper->opt, helper->idx_argp, helper->idx_ret, idx_val);

        /* if narg == -'+', check it at last. */
    }
    else
    {
        return luaL_error(L, "unknown narg value `%d`.", helper->narg);
    }

    return 0;
}

static int _infra_argp_parse_args_onpass_multi(lua_State* L,
    infra_argp_helper_t* helper, const char* k, int idx_beg, size_t num)
{
    idx_beg = lua_absindex(L, idx_beg);

    helper->opt = _infra_argp_find_option(L, helper->self, k);
    if (helper->opt->c_narg == 0)
    {
        lua_pushnil(L);
        _infra_argp_handle_option(L, helper->opt, helper->idx_argp, helper->idx_ret, -1);
        lua_pop(L, 1);

        /* No need value, reset. */
        helper->opt = NULL;
        return 0;
    }

    helper->narg = helper->opt->c_narg;

    size_t i;
    for (i = 0; i < num; i++)
    {
        /* Only fill necessary values. */
        if (helper->opt != NULL)
        {
            _infra_argp_parse_args_remain(L, helper, idx_beg + (int)i);
        }
    }

    return 0;
}

static int _infra_argp_parse_args_onepass_pure(lua_State* L,
    infra_argp_helper_t* helper, const char* v)
{
    return _infra_argp_parse_args_onpass_multi(L, helper, v, 0, 0);
}

static int _infra_argp_parse_args_onepass_shortopt(lua_State* L,
    infra_argp_helper_t* helper, const char* v)
{
    if (v[2] == '\0')
    {
        return _infra_argp_parse_args_onepass_pure(L, helper, v);
    }

    int sp = lua_gettop(L);

    lua_pushfstring(L, "-%c", v[1]); // sp+1
    lua_pushstring(L, v + 2); // sp+2
    _infra_argp_parse_args_onpass_multi(L, helper, lua_tostring(L, sp + 1), sp + 2, 1);

    lua_settop(L, sp);
    return 0;
}

static int _infra_argp_parse_args_onepass_longopt(lua_State* L,
    infra_argp_helper_t* helper, const char* v)
{
    char* p = strchr(v, '=');
    if (p == NULL)
    {
        return _infra_argp_parse_args_onepass_pure(L, helper, v);
    }

    int sp = lua_gettop(L);

    lua_pushlstring(L, v, p - v); // sp+1
    lua_pushstring(L, p + 1); // sp+2

    _infra_argp_parse_args_onpass_multi(L, helper, lua_tostring(L, sp + 1), sp + 2, 1);

    lua_settop(L, sp);
    return 0;
}

static int _infra_argp_parse_args_onepass(lua_State* L,
    infra_argp_helper_t* helper, int idx_val)
{
    idx_val = lua_absindex(L, idx_val);

    const char* v = luaL_checkstring(L, idx_val);

    if (helper->opt != NULL)
    {
        return _infra_argp_parse_args_remain(L, helper, idx_val);
    }

    /* Positional argument. */
    if (v[0] != '-')
    {
        return _infra_argp_parse_args_onepass_pure(L, helper, v);
    }

    if (v[1] != '-')
    {
        return _infra_argp_parse_args_onepass_shortopt(L, helper, v);
    }

    return _infra_argp_parse_args_onepass_longopt(L, helper, v);
}

static int _infra_argp_parse_args(lua_State* L)
{
    int sp = lua_gettop(L);
    infra_argp_t* self = luaL_checkudata(L, 1, INFRA_ARGPARSER_NAME);

    lua_newtable(L); // sp+1

    infra_argp_helper_t helper;
    memset(&helper, 0, sizeof(helper));
    helper.self = self;
    helper.idx_argp = 1;
    helper.idx_ret = sp + 1;

    for (helper.pos_arg = 1; lua_geti(L, 2, helper.pos_arg) == LUA_TSTRING; helper.pos_arg++) // sp+2
    {
        _infra_argp_parse_args_onepass(L, &helper, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (helper.narg > 0)
    {
        return luaL_error(L, "%s: error: missing argument to `%s`.",
            self->prog, helper.opt->names[0]);
    }

    _infra_argp_check_args_valid(L, self, sp + 1);

    return 1;
}

static int _infra_argp_init(lua_State* L, infra_argp_t* self, int idx)
{
    if (lua_type(L, idx) == LUA_TNIL)
    {
        return 0;
    }

    if (lua_getfield(L, idx, "prog") == LUA_TSTRING)
    {
        if ((self->prog = strdup(lua_tostring(L, -1))) == NULL)
        {
            goto error_oom;
        }
    }
    else
    {
        lua_pushcfunction(L, infra_f_basename.addr);
        lua_pushcfunction(L, infra_f_exepath.addr);
        lua_call(L, 0, 1);
        lua_call(L, 1, 1);

        self->prog = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);

        INFRA_CHECK_OOM(L, self->prog);
    }
    lua_pop(L, 1);

    if (lua_getfield(L, idx, "description") == LUA_TSTRING)
    {
        if ((self->desc = strdup(lua_tostring(L, -1))) == NULL)
        {
            goto error_oom;
        }
    }
    lua_pop(L, 1);

    if (lua_getfield(L, idx, "epilog") == LUA_TSTRING)
    {
        if ((self->epilog = strdup(lua_tostring(L, -1))) == NULL)
        {
            goto error_oom;
        }
    }
    lua_pop(L, 1);

    return 0;

error_oom:
    return luaL_error(L, INFRA_LUA_ERRMSG_OOM);
}

static int _infra_argparser(lua_State* L)
{
    infra_argp_t* self = lua_newuserdata(L, sizeof(infra_argp_t));
    memset(self, 0, sizeof(*self));
    ev_list_init(&self->arg_table);

    static const luaL_Reg s_meta[] = {
        { "__gc",               _infra_argp_meta_gc },
        { NULL,                 NULL },
    };
    static const luaL_Reg s_method[] = {
        { "add_argument",       _infra_argp_add_argument },
        { "parse_args",         _infra_argp_parse_args },
        { "format_help",        _infra_argp_format_help },
        { NULL,                 NULL },
    };
    if (luaL_newmetatable(L, INFRA_ARGPARSER_NAME) != 0)
    {
        luaL_setfuncs(L, s_meta, 0);

        /* metatable.__index = s_method */
        luaL_newlib(L, s_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);

    _infra_argp_init(L, self, 1);

    return 1;
}

const infra_lua_api_t infra_f_argparser = {
"argparser", _infra_argparser, 0,
"A easy way to define and parser command-line arguments.",

"[SYNOPSIS]\n"
"argparser argparser(table config)\n"
"\n"
"[DESCRIPTION]\n"
"Create a util that parser command-line arguments.\n"
"\n"
"The parameter `config` is a table that have following field:\n"
"+ `prog`: The name of the program. Default: `basename(exepath())`\n"
"+ `description`: Text to display before the argument help. Default: nil.\n"
"+ `epilog`: Text to display after the argument help. Default: nil.\n"
"\n"
"The util have following meta-method:\n"
"\n"
"+ add_argument(self, string name, table opt):\n"
"  Attaches individual argument specifications to the parser.\n"
"  + `name`: Either a name or a list of option strings, e.g. `foo` or `-f, --foo`.\n"
"  + `opt`: A table contains optional configuration:\n"
"    + `action`: string for function. Default: \"store\".\n"
"      + \"store\": This just stores the argument's value.\n"
"      + \"append\": This stores a list, and appends each argument value to the\n"
"        list.\n"
"      + \"count\": This counts the number of times a keyword argument occurs.\n"
"      + \"help\": This prints a complete help message for all the options in\n"
"        the current parser and then exits.\n"
"      + function: the protocol is `any (*)(self, any oldval, any newval)`.\n"
"    + `type`: Automatically convert an argument to the given type. Accept\n"
"      \"string\", \"number\", or a function that create object.\n"
"    + `choices`: Array of limit values to a specific set of choices.\n"
"    + `default`£º Default value used when an argument is not provided.\n"
"    + `nargs`: Usually associate a single command-line argument with a single\n"
"      action to be taken. The nargs keyword argument associates a different\n"
"      number of command-line arguments with a single action. The supported\n"
"      values are:\n"
"      + N (an integer): N arguments from the command line will be gathered\n"
"        together into a list.\n"
"      + '?': One argument will be consumed from the command line if possible,\n"
"        and produced as a single item. If no command-line argument is present,\n"
"        the value from default will be produced.\n"
"      + '*': All command-line arguments present are gathered into a list.\n"
"      + '+': Just like '*', all command-line args present are gathered into a\n"
"        list. Additionally, an error message will be generated if there wasn't\n"
"        at least one command-line argument present.\n"
"    + `required`: boolean. Whether or not the command-line option may be\n"
"      omitted. Default: false.\n"
"    + `help`: A brief description of what the argument does.\n"
"\n"
"+ table parse_args(self, table arguments)\n"
"  Convert argument strings to objects and assign them as attributes of the\n"
"  table. Return the populated table.\n"
"  + `arguments`: List of strings to parse.\n"
"\n"
"+ format_help(self)\n"
"  Return a string containing a help message, including the program usage and\n"
"  information about the arguments registered.\n"
};
