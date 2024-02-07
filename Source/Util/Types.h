// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef union color
{
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint8_t  arr[4];
    uint32_t raw;
} color_t;

_Static_assert(sizeof(color_t) == 4, "invalid size");

typedef struct vector2f
{
    float x;
    float y;
} vector2f_t;

typedef struct vector3f
{
    float x;
    float y;
    float z;
} vector3f_t;

typedef struct vector3l
{
    long x;
    long y;
    long z;
} vector3l_t;

typedef struct vector3i
{
    int x;
    int y;
    int z;
} vector3i_t;

typedef struct quad3d
{
    vector3f_t from;
    vector3f_t to;
} quad3d_t;

#endif /* TYPES_H */
