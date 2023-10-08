#include "osSemaphore.h"
#include "osKernel.h"

void osSemaphoreInit(osSemaphoreObject* semaphore, const uint32_t maxCount, const uint32_t count)
{
    semaphore->maxCount = maxCount;
    semaphore->count = count;

    /* Is a binary semaphore so I'm going to use the variable Locked only */
    /* If semaphore is 0 is given, if it is 1 is taken*/
    semaphore->locked = 0;
}



bool osSemaphoreTake(osSemaphoreObject* semaphore)
{
	osEnterCriticalSection();

	/* TODO: Implement a blocking API -> blockTaskFromSem (DONE)*/
    if (semaphore->locked == 1)
    {
        blockTaskFromSem(semaphore);
        osExitCriticalSection();
        
        return false;
    }
    else
    {
        semaphore->locked = 1;
    }

    osExitCriticalSection();
    return true;
}



void osSemaphoreGive(osSemaphoreObject* semaphore)
{
	/*TODO: Implemented a unblocking API -> checkBlockedTaskFromSem (DONE) */
	osEnterCriticalSection();

    semaphore->locked = 0;
    
    checkBlockedTaskFromSem(semaphore);

    osExitCriticalSection();
}
