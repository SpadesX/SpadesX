// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef TYPES_H
#define TYPES_H

#define STATUS(msg)  printf("STATUS: " msg "\n")
#define WARNING(msg) printf("WARNING: " msg "\n")
#define ERROR(msg)   fprintf(stderr, "ERROR: " msg "\n");

typedef unsigned char          uint8;
typedef unsigned short         uint16;
typedef unsigned int           uint32;
typedef unsigned long long int uint64;

/**
 * @brief 3-byte color
 * @details [0] = A, [1] = R, [2] = G [3] = B
 *
 */
typedef uint8 Color3i[3];

typedef uint8 Color4i[4];

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

typedef struct
{
    long x;
    long y;
    long z;
} Vector3l;

typedef struct
{
    int x;
    int y;
    int z;
} Vector3i;

#endif /* TYPES_H */
