// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef QUEUE_H
#define QUEUE_H

#include <Util/Types.h>

/**
 * @brief Queue node - minimal linked list
 *
 */
typedef struct queue
{
    uint8_t*      block;
    uint32_t      capacity;
    uint32_t      length;
    struct queue* next;
} queue_t;

/**
 * @brief Append queue node
 *
 * @param node Previous queue node
 * @param capacity Capacity of the queue node
 * @return Pointer to newly created queue node
 */
queue_t* queue_push(queue_t* node, uint32_t capacity);

/**
 * @brief Pop queue node from the front of queue
 *
 * @param node The first node in queue
 * @return Pointer to next queue node or null
 */
queue_t* queue_pop(queue_t* node);

#endif /* QUEUE_H */
