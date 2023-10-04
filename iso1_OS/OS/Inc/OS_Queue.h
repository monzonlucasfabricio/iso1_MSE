#ifndef __OSQUEUE_H__
#define __OSQUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define MAX_SIZE_QUEUE  128     // Maximum buffer size of the queue

/**
 * @brief Data structure queue.
 */
typedef struct
{
	uint32_t dataSize;
	uint32_t size;
	uint32_t front;
	uint32_t rear;
    void *elements[MAX_SIZE_QUEUE];
}osQueueObject;

/**
 * @brief Initialize the queue.
 *
 * @param[in, out]  queue       Queue object.
 * @param[in]       dataSize    Data size of the queue.
 *
 * @return Returns true if was success in otherwise false.
 */
bool osQueueInit(osQueueObject* queue, const uint32_t dataSize);

/**
 * @brief Send data to the queue.
 *
 * @param[in, out]  queue   Queue object.
 * @param[in, out]  data    Data sent to the queue.
 * @param[in]       timeout Number of ticks to wait before blocking the task..
 *
 * @return Returns true if it could be put in the queue
 * in otherwise false.
 */
bool osQueueSend(osQueueObject* queue, const void* data, const uint32_t timeout);

/**
 * @brief Receive data to the queue.
 *
 * @param[in, out]  queue   Queue object.
 * @param[in, out]  buffer  Buffer to  save the data read from the queue.
 * @param[in]       timeout Number of ticks to wait before blocking the task..
 *
 * @return Returns true if it was possible to take it out in the queue
 * in otherwise false.
 */
bool osQueueReceive(osQueueObject* queue, void* buffer, const uint32_t timeout);

#ifdef __cplusplus
}
#endif


#endif // INC_OSQUEUE_H
