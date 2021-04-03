#ifndef TYPES_H
#define TYPES_H

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

/**
 * @brief 3-byte color
 * @details [0] = R, [1] = G, [2] = B
 *
 */
typedef uint8 Color3i[3];

typedef struct
{
    float x;
    float y;
} Vector2f;

typedef struct
{
    Vector2f from;
    Vector2f to;
} Quad2D;

typedef struct
{
    float x;
    float y;
    float z;
} Vector3f;

#endif /* TYPES_H */
