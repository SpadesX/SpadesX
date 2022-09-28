// Copyright CircumScriptor and DarkNeutrino 2021
#include "Queue.h"

#include <stdlib.h>

queue_t* queue_push(queue_t* node, uint32_t capacity)
{
    // Allocate everything in one call
    void* memory = malloc(sizeof(queue_t) + capacity);

    queue_t* next  = (queue_t*) memory;
    next->next     = NULL;
    next->capacity = capacity;
    next->length   = 0;
    next->block    = (uint8_t*) memory + sizeof(queue_t);
    if (node != NULL) {
        node->next = next;
    }
    return next;
}

queue_t* queue_pop(queue_t* node)
{
    if (node != NULL) {
        queue_t* next = node->next;
        free(node);
        return next;
    }
    return NULL;
}
