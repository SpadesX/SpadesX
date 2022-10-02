#ifndef GRENADESTRUCT_H
#define GRENADESTRUCT_H

#include <Util/Types.h>

typedef struct grenade
{
    uint8_t         sent;
    float           fuse;
    uint8_t         exploded;
    vector3f_t      position;
    vector3f_t      velocity;
    uint64_t        time_since_sent;
    struct grenade *next, *prev;
} grenade_t;

#endif
