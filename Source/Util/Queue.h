#ifndef QUEUE_H
#define QUEUE_H

#include <Util/Types.h>

typedef struct queue
{
    uint8_t*      block;
    uint32_t      capacity;
    uint32_t      length;
    struct queue *next, *prev;
} queue_t;

#endif /* QUEUE_H */
