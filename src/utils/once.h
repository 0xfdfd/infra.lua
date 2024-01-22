#ifndef __INFRA_UTILS_ONCE_H__
#define __INFRA_UTILS_ONCE_H__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)

typedef struct infra_once_s {
	unsigned char ran;
	HANDLE event;
} infra_once_t;

#define INFRA_ONCE_INIT { 0, NULL }

#else

typedef pthread_once_t infra_once_t;

#define INFRA_ONCE_INIT PTHREAD_ONCE_INIT

#endif

/**
 * @brief Execute callback.
 */
typedef void (*infra_once_cb)(void);

/**
 * @brief Execute code only once.
 * @param[in] guard		Guard token which must initialized with #INFRA_ONCE_INIT.
 * @param[in] cb		Execute callback.
 */
API_LOCAL void infra_once(infra_once_t* guard, infra_once_cb cb);

#ifdef __cplusplus
}
#endif
#endif
