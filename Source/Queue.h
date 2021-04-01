#ifndef QUEUE_H
#define QUEUE_H

#include "Types.h"

/**
 * @brief Queue node - minimal linked list
 *
 */
typedef struct _Queue
{
    uint8*         block;
    uint32         capacity;
    uint32         length;
    struct _Queue* next;
} Queue;

/**
 * @brief Append queue node
 *
 * @param node Previous queue node
 * @param capacity Capacity of the queue node
 * @return Pointer to newly created queue node
 */
Queue* Push(Queue* node, uint32 capacity);

/**
 * @brief Pop queue node from the front of queue
 *
 * @param node The first node in queue
 * @return Pointer to next queue node or null
 */
Queue* Pop(Queue* node);

#endif /* QUEUE_H */
