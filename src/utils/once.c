#include "once.h"

#if defined(_WIN32)

#include <windows.h>

static void uv__once_inner(infra_once_t* guard, infra_once_cb callback)
{
	DWORD result;
	HANDLE existing_event, created_event;

	created_event = CreateEvent(NULL, 1, 0, NULL);
	if (created_event == 0)
	{
		/* Could fail in a low-memory situation? */
		abort();
	}

	existing_event = InterlockedCompareExchangePointer(&guard->event,
		created_event,
		NULL);

	if (existing_event == NULL)
	{
		/* We won the race */
		callback();

		result = SetEvent(created_event);
		assert(result);
		guard->ran = 1;

	}
	else
	{
		/* We lost the race. Destroy the event we created and wait for the existing
		 * one to become signaled. */
		CloseHandle(created_event);
		result = WaitForSingleObject(existing_event, INFINITE);
		assert(result == WAIT_OBJECT_0);
	}
}

void infra_once(infra_once_t* guard, infra_once_cb cb)
{
	/* Fast case - avoid WaitForSingleObject. */
	if (guard->ran)
	{
		return;
	}

	uv__once_inner(guard, cb);
}

#else

void infra_once(infra_once_t* guard, infra_once_cb cb)
{
	if (pthread_once(guard, cb))
	{
		abort();
	}
}

#endif
