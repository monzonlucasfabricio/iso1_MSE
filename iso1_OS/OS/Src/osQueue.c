#include "osQueue.h"
#include "osKernel.h"
#include <stdlib.h>
#include <string.h>

/*
 * Insert operations
 * | a |   |   |   | front = 0 ; back = 0 ; size = 1
 * | a | b |   |   | front = 0 ; back = 1 ; size = 2
 * | a | b | c |   | front = 0 ; back = 2 ; size = 3
 * | a | b | c | d | front = 0 ; back = 3 ; size = 4
 *
 * Extract operations
 * |   | b | c | d | front = 1 ; back = 3 ; size = 3
 * |   |   | c | d | front = 2 ; back = 3 ; size = 2
 * |   |   |   | d | front = 3 ; back = 3 ; size = 1
 * | e |   |   | d | front = 3 ; back = 0 ; size = 2
 * | e |   |   |   | front = 0 ; back = 0 ; size = 1
 * |   |   |   |   | front = 1 ; back = 0 ; size = 0
 *
*/

bool osQueueInit(osQueueObject* queue, const u32 dataSize)
{
    /* Init the queue */
    if (NULL != queue)
    {
        queue->dataSize = dataSize;
        queue->size = 0;
        queue->back = -1;
        queue->front = 0;
        return true;
    }
    return false;
}


bool osQueueSend(osQueueObject* queue, const void* data, const u32 timeout)
{
    /* Queue is FULL we need to block the task until there is a place in the queue*/
    if (queue->size >= MAX_SIZE_QUEUE && osGetStatus() == OS_STATUS_RUNNING)
    {
    	osEnterCriticalSection();
        blockTaskFromQueue(queue, 1); // 1 means that is blocking from the sender
        osExitCriticalSection();
    }

    if (queue->size < MAX_SIZE_QUEUE)
    {  
    	osEnterCriticalSection();
        /* If we have a place we put the pointer on that place */
        queue->back = (queue->back + 1)%MAX_SIZE_QUEUE;

        /* TODO: Change this for a static implementation. */
        queue->elements[queue->back] = malloc(queue->dataSize);
        memcpy(queue->elements[queue->back], data, queue->dataSize);

        queue->size++;

        if (queue->size == 1) checkBlockedTaskFromQueue(queue, 1); // Check only in the limit
        osExitCriticalSection();
    }

    return true;
}


bool osQueueReceive(osQueueObject* queue, void* buffer, const u32 timeout)
{
	if (queue->size == 0 && osGetStatus() == OS_STATUS_RUNNING)
	{
		osEnterCriticalSection();
		blockTaskFromQueue(queue,0); // 0 means that is blocking form receiver
		osExitCriticalSection();
	}

	if (queue->size > 0)
    {
		osEnterCriticalSection();
		/* TODO: Change this for a static implementation */
		memcpy(buffer, queue->elements[queue->front], queue->dataSize);
		free(queue->elements[queue->front]);

        queue->front = (queue->front + 1)%MAX_SIZE_QUEUE;
        queue->size--;

        if (queue->size == MAX_SIZE_QUEUE - 1) checkBlockedTaskFromQueue(queue, 0); // Check only in the limit
        osExitCriticalSection();
    }

    return true;
}
