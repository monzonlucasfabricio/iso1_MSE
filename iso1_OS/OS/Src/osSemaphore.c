#include "osSemaphore.h"
#include "osKernel.h"

void osSemaphoreInit(osSemaphoreObject* semaphore, const uint32_t maxCount, const uint32_t count)
{
    semaphore->maxCount = maxCount;
    semaphore->count = count;

    /* Is a binary semaphore so I'm going to use the variable Locked only */
    /* If semaphore is 0 is given, if it is 1 is taken*/
    semaphore->locked = 1; // Start with semaphore taken
}



bool osSemaphoreTake(osSemaphoreObject* semaphore)
{
	bool ret = false;
	/* TODO: Implement a blocking API -> blockTaskFromSem (DONE)*/
	if (semaphore->locked == 1 && osGetStatus() == OS_STATUS_RUNNING)
	{
		ENTER_CRITICAL_SECTION
		blockTaskFromSem(semaphore);
		EXIT_CRITICAL_SECTION
	}

	if (semaphore->locked == 0)
	{
		ENTER_CRITICAL_SECTION
		semaphore->locked = 1;
		EXIT_CRITICAL_SECTION
		ret = true;
	}

    return ret;
}



void osSemaphoreGive(osSemaphoreObject* semaphore)
{
	/*TODO: Implemented a unblocking API -> checkBlockedTaskFromSem (DONE) */
	ENTER_CRITICAL_SECTION

    semaphore->locked = 0;
    
    checkBlockedTaskFromSem(semaphore);

    EXIT_CRITICAL_SECTION
}
