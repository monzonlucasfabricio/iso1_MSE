#include "osSemaphore.h"
#include "osKernel.h"

void osSemaphoreInit(osSemaphoreObject* semaphore, const uint32_t maxCount, const uint32_t count)
{
    semaphore->maxCount = maxCount;
    semaphore->count = count;

    /* Is a binary semaphore so I'm going to use the variable Locked only */
    /* If semaphore is 0 is given, if it is 1 is taken*/
    semaphore->locked = 1;
}



bool osSemaphoreTake(osSemaphoreObject* semaphore)
{

	/* TODO: Implement a blocking API -> blockTaskFromSem (DONE)*/
	if (osGetStatus() == OS_STATUS_RUNNING)
	{
		if (semaphore->locked == 1)
		{
			osEnterCriticalSection();
			blockTaskFromSem(semaphore);
			osExitCriticalSection();
		}

		if (semaphore->locked == 0)
		{
			osEnterCriticalSection();
			semaphore->locked = 1;
			osExitCriticalSection();
		}
	}
    return true;
}



void osSemaphoreGive(osSemaphoreObject* semaphore)
{
	/*TODO: Implemented a unblocking API -> checkBlockedTaskFromSem (DONE) */
	if ( osGetStatus() == OS_STATUS_RUNNING) osEnterCriticalSection();

    semaphore->locked = 0;
    
    checkBlockedTaskFromSem(semaphore);

    if ( osGetStatus() == OS_STATUS_RUNNING) osExitCriticalSection();
}
