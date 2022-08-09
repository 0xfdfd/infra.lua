#ifndef __INFRA_API_OBJECT_H__
#define __INFRA_API_OBJECT_H__

#include "lua_api.h"
#include "utils/list.h"

/**
 * @brief Infra object table.
 */
#define LUA_OBJECT_TABLE(XX)    \
    XX(INFRA_INT64)             \
    XX(INFRA_UINT64)            \
    XX(INFRA_LIST)              \
    XX(INFRA_JSON)              \
    XX(INFRA_MAP)               \
    XX(INFRA_MAP_VISITOR)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Infra object type.
 */
typedef enum infra_object_type_e
{
    INFRA__OBJECT_BEG = 1000,   /**< Avoid conflict with lua native type */
#define EXPAND_LUA_OBJECT_TABLE_AS_ENUM(name)   name,
    LUA_OBJECT_TABLE(EXPAND_LUA_OBJECT_TABLE_AS_ENUM)
#undef EXPAND_LUA_OBJECT_TABLE_AS_ENUM
    INFRA__OBJECT_END,
} infra_object_type_t;

/**
 * @brief Infra runtime.
 * @note Each Lua VM have one runtime instance.
 */
typedef struct infra_vm_runtime_s
{
    ev_list_node_t      node;           /**< Node for #infra_runtime_t */
    ev_list_t           object_list;    /**< Infra object list */
} infra_vm_runtime_t;

/**
 * @brief Infra object.
 */
typedef struct infra_object_s
{
    ev_list_node_t      node;           /**< List node for #infra_vm_runtime_t or #infra_runtime_t */
    infra_object_type_t type;           /**< Object type */
    infra_vm_runtime_t* vm;             /**< Lua VM id */
} infra_object_t;

/**
 * @brief Initialize and register object.
 * @param[out] object   Object handle.
 * @param[in] L         Lua VM.
 * @param[in] type      Object type.
 */
API_LOCAL void infra_init_object(infra_object_t* object, lua_State *L, infra_object_type_t type);

/**
 * @brief Cleanup and unregister object.
 * @param[in] object    Object handle.
 */
API_LOCAL void infra_exit_object(infra_object_t* object);

/**
 * @brief Convert #infra_object_type_t to C string.
 * @param[in] type  Object type name. Also compatible with lua native type.
 * @return          Type string.
 */
API_LOCAL const char* infra_type_name(int type);

/**
 * @brief Get object type at stack index \p idx.
 *
 * This function extend the capability of #lua_type(). If object at \p idx is a
 * registered infra object, return the equal #infra_object_type_t, otherwise
 * return the equal value of lua_type().
 *
 * @param[in] L     Lua VM.
 * @param[in] idx   Stack index.
 * @return          #infra_object_type_t
 */
API_LOCAL int infra_type(lua_State* L, int idx);

#ifdef __cplusplus
}
#endif

#endif
