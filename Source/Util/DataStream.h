// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef DATASTREAM_H
#define DATASTREAM_H

#include "Enums.h"
#include "Types.h"

#define ACCESS_CHECK(stream, size)             \
    if (stream->pos + size > stream->length) { \
        return 0;                              \
    }

#define ACCESS_CHECK_N(stream, size)           \
    if (stream->pos + size > stream->length) { \
        return;                                \
    }

typedef struct
{
    uint8* data;
    uint32 length;
    uint32 pos;
} DataStream;

void CreateDataStream(DataStream* stream, uint32 length);

void DestroyDataStream(DataStream* stream);

inline uint32 DataLeft(DataStream* stream)
{
    return (stream->pos < stream->length) ? stream->length - stream->pos : 0;
}

inline void StreamSkip(DataStream* stream, uint32 skip)
{
    stream->pos = (stream->pos + skip < stream->length) ? stream->pos + skip : stream->length;
}

inline uint8 ReadByte(DataStream* stream)
{
    ACCESS_CHECK(stream, 1);
    return stream->data[stream->pos++];
}

inline uint16 ReadShort(DataStream* stream)
{
    ACCESS_CHECK(stream, 2);
    uint16 value = 0;
    value |= ((uint16) stream->data[stream->pos++]);
    value |= ((uint16) stream->data[stream->pos++]) << 8;
    return value;
}

inline uint32 ReadInt(DataStream* stream)
{
    ACCESS_CHECK(stream, 4);
    uint32 value = 0;
    value |= ((uint32) stream->data[stream->pos++]);
    value |= ((uint32) stream->data[stream->pos++]) << 8;
    value |= ((uint32) stream->data[stream->pos++]) << 16;
    value |= ((uint32) stream->data[stream->pos++]) << 24;
    return value;
}

inline float ReadFloat(DataStream* stream)
{
    union
    {
        float  f;
        uint32 v;
    } u;
    u.v = ReadInt(stream);
    return u.f;
}

inline void ReadColor3i(DataStream* stream, Color3i color)
{
    ACCESS_CHECK_N(stream, 3);
    color.colorArray[R_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[G_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[B_CHANNEL] = stream->data[stream->pos++];
}

inline void ReadColor4i(DataStream* stream, Color4i color)
{
    ACCESS_CHECK_N(stream, 4);
    color.colorArray[A_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[R_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[G_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[B_CHANNEL] = stream->data[stream->pos++];
}

void ReadArray(DataStream* stream, void* output, uint32 length);

inline void WriteByte(DataStream* stream, uint8 value)
{
    ACCESS_CHECK_N(stream, 1);
    stream->data[stream->pos++] = value;
}

inline void WriteShort(DataStream* stream, uint16 value)
{
    ACCESS_CHECK_N(stream, 2);
    stream->data[stream->pos++] = (uint8) value;
    stream->data[stream->pos++] = (uint8) (value >> 8);
}

inline void WriteInt(DataStream* stream, uint32 value)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = (uint8) value;
    stream->data[stream->pos++] = (uint8) (value >> 8);
    stream->data[stream->pos++] = (uint8) (value >> 16);
    stream->data[stream->pos++] = (uint8) (value >> 24);
}

inline void WriteFloat(DataStream* stream, float value)
{
    union
    {
        float  f;
        uint32 v;
    } u;
    u.f = value;
    WriteInt(stream, u.v);
}

inline void WriteVector3f(DataStream* stream, Vector3f vector)
{
    WriteFloat(stream, vector.x);
    WriteFloat(stream, vector.y);
    WriteFloat(stream, vector.z);
}

inline void WriteColor3i(DataStream* stream, Color3i color)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = color.colorArray[R_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[G_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[B_CHANNEL];
}

inline void WriteColor4i(DataStream* stream, Color4i color)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = color.colorArray[A_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[R_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[G_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[B_CHANNEL];
}

inline void WriteColor3iv(DataStream* stream, uint8 r, uint8 g, uint8 b)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
}

inline void WriteColor4iv(DataStream* stream, uint8 a, uint8 r, uint8 g, uint8 b)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
    stream->data[stream->pos++] = a;
}

void WriteArray(DataStream* stream, const void* array, uint32 length);

#endif /* DATASTREAM_H */
