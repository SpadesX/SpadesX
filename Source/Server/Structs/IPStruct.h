#ifndef IPSTRUCT_H
#define IPSTRUCT_H

#include <Util/Types.h>

typedef struct
{
    union
    {
        uint8_t  ip[4];
        uint32_t ip32;
    };
    uint8_t cidr;
} ip_t;

#endif
