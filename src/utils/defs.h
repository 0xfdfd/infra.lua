#ifndef __INFRA_UTILS_DEFS_H__
#define __INFRA_UTILS_DEFS_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT
#    define _WIN32_WINNT   0x0600
#endif

#if defined(_WIN32)
#   if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
typedef intptr_t ssize_t;
#       define SSIZE_MAX INTPTR_MAX
#       define _SSIZE_T_
#       define _SSIZE_T_DEFINED
#   endif
#else
#   include <sys/types.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get the number of element in array.
 * @param[in] a Array.
 * @return      Element count.
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

 /**
  * @brief Align \p size to \p align, who's value is larger or equal to \p size
  *   and can be divided with no remainder by \p align.
  * @note \p align must equal to 2^n
  */
#define ALIGN_SIZE(size, align) \
    (((uintptr_t)(size) + ((uintptr_t)(align) - 1)) & ~((uintptr_t)(align) - 1))

/**
 * @brief Default as local API.
 *
 * On gcc/clang, non-static C function are export by default. However most of
 * library function should not be exported.
 *
 * Use this macro to ensure these functions is hidden for user.
 */
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32)
#   define API_LOCAL    __attribute__((visibility ("hidden")))
#else
#   define API_LOCAL
#endif

#if !defined(offsetof)
#   define offsetof(s, m) ((size_t)&(((s*)0)->m))
#endif

#if !defined(container_of)
#   if defined(__GNUC__) || defined(__clang__)
#       define container_of(ptr, type, member)   \
            ({ \
                const typeof(((type *)0)->member)*__mptr = (ptr); \
                (type *)((char *)__mptr - offsetof(type, member)); \
            })
#   else
#       define container_of(ptr, type, member)   \
            ((type *) ((char *) (ptr) - offsetof(type, member)))
#   endif
#endif

///////////////////////////////////////////////////////////////////////////////
// ERROR
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>

/**
 * @{
 */
#define INFRA_EOF		(-4095)
#define INFRA_UNKNOWN	(-4094)
#define INFRA_ETIMEDOUT	(-ETIMEDOUT)
/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
