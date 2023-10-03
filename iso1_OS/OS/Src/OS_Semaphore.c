#include "OS_Semaphore.h"


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

    while(semaphore->locked)
    {
        // Wait until semaphore is released
    }

    semaphore->locked = 1;

    return true;
}



void osSemaphoreGive(osSemaphoreObject* semaphore)
{
    semaphore->locked = 0;
}
