#include "mutex.h"

#if defined(_WIN32)

int infra_mutex_init(infra_mutex_t* mutex)
{
	InitializeCriticalSection(mutex);
	return 0;
}

void infra_mutex_exit(infra_mutex_t* mutex)
{
	DeleteCriticalSection(mutex);
}

void infra_mutex_enter(infra_mutex_t* mutex)
{
	EnterCriticalSection(mutex);
}

void infra_mutex_leave(infra_mutex_t* mutex)
{
	LeaveCriticalSection(mutex);
}

#else

int infra_mutex_init(infra_mutex_t* mutex)
{
	pthread_mutexattr_t attr;
	int err;

	if (pthread_mutexattr_init(&attr))
		abort();

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))
		abort();

	err = pthread_mutex_init(mutex, &attr);

	if (pthread_mutexattr_destroy(&attr))
		abort();

	return infra_translate_sys_error(err);
}

void infra_mutex_exit(infra_mutex_t* mutex)
{
	if (pthread_mutex_destroy(mutex))
	{
		abort();
	}
}

void infra_mutex_enter(infra_mutex_t* mutex)
{
	if (pthread_mutex_lock(mutex))
	{
		abort();
	}
}

void infra_mutex_leave(infra_mutex_t* mutex)
{
	if (pthread_mutex_unlock(mutex))
	{
		abort();
	}
}

#endif
