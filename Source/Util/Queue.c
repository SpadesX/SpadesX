// Copyright CircumScriptor and DarkNeutrino 2021
#include "Queue.h"

#include <stdlib.h>

queue_t* queue_push(queue_t* node, uint32_t capacity)
{
    queue_t* next  = (queue_t*) malloc(sizeof(queue_t));
    next->next     = NULL;
    next->capacity = capacity;
    next->length   = 0;
    next->block    = (uint8_t*) malloc(capacity);
    if (node != NULL) {
        node->next = next;
    }
    return next;
}

queue_t* queue_pop(queue_t* node)
{
    if (node != NULL) {
        queue_t* next = node->next;
        free(node->block);
        free(node);
        return next;
    }
    return NULL;
}
