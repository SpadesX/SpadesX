#ifndef BLOCKSTRUCT_H
#define BLOCKSTRUCT_H

#include <Util/Types.h>

typedef struct block_node
{
    struct block_node* next;
    vector3i_t         position;
    vector3i_t         position_end;
    color_t            color;
    uint8_t            type;
    uint8_t            sender_id;
} block_node_t;

#endif
