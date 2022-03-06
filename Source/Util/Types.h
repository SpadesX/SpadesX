// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef TYPES_H
#define TYPES_H

#define LOG__INT(msg, ...) printf(msg "%s", __VA_ARGS__);
#define LOGF__INT(msg, ...) fprintf(stderr, msg "%s", __VA_ARGS__);

#define LOG_DEBUG(...)  LOG__INT("DEBUG: " __VA_ARGS__, "\n")
#define LOG_INFO(...)  LOG__INT("INFO: " __VA_ARGS__, "\n")
#define LOG_STATUS(...)  LOG__INT("STATUS: " __VA_ARGS__, "\n")
#define LOG_WARNING(...) LOG__INT("WARNING: " __VA_ARGS__, "\n")
#define LOG_ERROR(...)   LOGF__INT("ERROR: " __VA_ARGS__, "\n")


typedef unsigned char          uint8;
typedef unsigned short         uint16;
typedef unsigned int           uint32;
typedef unsigned long long int uint64;

typedef union
{
    uint8 colorArray[4];
    uint32 color;
} Color4i;

typedef union
{
    uint8 colorArray[3];
    uint32 color; //Bitshift required before usage
} Color3i;

typedef struct
{
    float x;
    float y;
} Vector2f;

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

typedef struct
{
    Vector3f from;
    Vector3f to;
} Quad3D;

#endif /* TYPES_H */
