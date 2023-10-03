#ifndef INC_OSSEMAPHORE_H
#define INC_OSSEMAPHORE_H

#include "OS_Core.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_DELAY 0xFFFFFFFF

typedef struct
{
    u32  maxCount; 
    u32  count;
    u8   locked;

}osSemaphoreObject;

/**
 * @brief Initializes semaphore binary or not.
 *
 * @param[in,out]   semaphore   Semaphore handler.
 * @param[in]       maxCount    Maximum count value that can be reached.
 * @param[in]       count       The count value assigned to the semaphore when it is created.
 */
void osSemaphoreInit(osSemaphoreObject* semaphore, const u32 maxCount, const u32 count);

/**
 * @brief Take semaphore.
 *
 * @param[in,out]   semaphore   Semaphore handler.
 *
 * @return Returns true if the semaphore could be taken.
 */
bool osSemaphoreTake(osSemaphoreObject* semaphore);

/**
 * @brief Give semaphore.
 *
 * @param[in,out]   semaphore   Semaphore handler.
 */
void osSemaphoreGive(osSemaphoreObject* semaphore);


#endif // INC_OSSEMAPHORE_H