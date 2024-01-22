#ifndef __INFRA_UTILS_MUTEX_H__
#define __INFRA_UTILS_MUTEX_H__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)

#include <windows.h>

typedef CRITICAL_SECTION infra_mutex_t;

#else

#include <pthread.h>

typedef pthread_mutex_t infra_mutex_t;

#endif

API_LOCAL int infra_mutex_init(infra_mutex_t* mutex);
API_LOCAL void infra_mutex_exit(infra_mutex_t* mutex);
API_LOCAL void infra_mutex_enter(infra_mutex_t* mutex);
API_LOCAL void infra_mutex_leave(infra_mutex_t* mutex);

#ifdef __cplusplus
}
#endif
#endif
