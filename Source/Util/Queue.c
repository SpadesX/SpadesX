#include "Queue.h"

#include <stdlib.h>

Queue* Push(Queue* node, uint32 capacity)
{
    Queue* next    = (Queue*) malloc(sizeof(Queue));
    next->next     = NULL;
    next->capacity = capacity;
    next->length   = 0;
    next->block    = (uint8*) malloc(capacity);
    if (node != NULL) {
        node->next = next;
    }
    return next;
}

Queue* Pop(Queue* node)
{
    if (node != NULL) {
        Queue* next = node->next;
        free(node->block);
        free(node);
        return next;
    }
    return NULL;
}
