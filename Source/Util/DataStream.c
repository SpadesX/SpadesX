// Copyright CircumScriptor and DarkNeutrino 2021
#include "DataStream.h"

#include <stdlib.h>
#include <string.h>

void CreateDataStream(DataStream* stream, uint32 length)
{
    stream->data   = malloc(length);
    stream->length = length;
    stream->pos    = 0;
}

void DestroyDataStream(DataStream* stream)
{
    if (stream->data != NULL) {
        free(stream->data);
    }
}

uint32 DataLeft(DataStream* stream)
{
    return (stream->pos < stream->length) ? stream->length - stream->pos : 0;
}

void StreamSkip(DataStream* stream, uint32 skip)
{
    stream->pos = (stream->pos + skip < stream->length) ? stream->pos + skip : stream->length;
}

uint8 ReadByte(DataStream* stream)
{
    ACCESS_CHECK(stream, 1);
    return stream->data[stream->pos++];
}

uint16 ReadShort(DataStream* stream)
{
    ACCESS_CHECK(stream, 2);
    uint16 value = 0;
    value |= ((uint16) stream->data[stream->pos++]);
    value |= ((uint16) stream->data[stream->pos++]) << 8;
    return value;
}

uint32 ReadInt(DataStream* stream)
{
    ACCESS_CHECK(stream, 4);
    uint32 value = 0;
    value |= ((uint32) stream->data[stream->pos++]);
    value |= ((uint32) stream->data[stream->pos++]) << 8;
    value |= ((uint32) stream->data[stream->pos++]) << 16;
    value |= ((uint32) stream->data[stream->pos++]) << 24;
    return value;
}

float ReadFloat(DataStream* stream)
{
    union
    {
        float  f;
        uint32 v;
    } u;
    u.v = 0;
    u.v |= ((uint32) stream->data[stream->pos++]);
    u.v |= ((uint32) stream->data[stream->pos++]) << 8;
    u.v |= ((uint32) stream->data[stream->pos++]) << 16;
    u.v |= ((uint32) stream->data[stream->pos++]) << 24;
    return u.f;
}

void ReadColor3i(DataStream* stream, Color3i color)
{
    ACCESS_CHECK_N(stream, 3);
    color.colorArray[R_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[G_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[B_CHANNEL] = stream->data[stream->pos++];
    Color3i temp                = color;
    color                       = temp; // Lets not error out over not using the damn variable but only setting it.
}

void ReadColor4i(DataStream* stream, Color4i color)
{
    ACCESS_CHECK_N(stream, 4);
    color.colorArray[A_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[R_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[G_CHANNEL] = stream->data[stream->pos++];
    color.colorArray[B_CHANNEL] = stream->data[stream->pos++];
    Color4i temp                = color;
    color                       = temp;
}

void ReadArray(DataStream* stream, void* output, uint32 length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(output, stream->data + stream->pos, length); //Replace me
    stream->pos += length;
}

void WriteArray(DataStream* stream, const void* array, uint32 length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(stream->data + stream->pos, array, length); //Replace me
    stream->pos += length;
}

void WriteByte(DataStream* stream, uint8 value)
{
    ACCESS_CHECK_N(stream, 1);
    stream->data[stream->pos++] = value;
}

void WriteShort(DataStream* stream, uint16 value)
{
    ACCESS_CHECK_N(stream, 2);
    stream->data[stream->pos++] = (uint8) value;
    stream->data[stream->pos++] = (uint8) (value >> 8);
}

void WriteInt(DataStream* stream, uint32 value)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = (uint8) value;
    stream->data[stream->pos++] = (uint8) (value >> 8);
    stream->data[stream->pos++] = (uint8) (value >> 16);
    stream->data[stream->pos++] = (uint8) (value >> 24);
}

void WriteFloat(DataStream* stream, float value)
{
    union
    {
        float  f;
        uint32 v;
    } u;
    u.f = value;
    WriteInt(stream, u.v);
}

void WriteVector3f(DataStream* stream, Vector3f vector)
{
    WriteFloat(stream, vector.x);
    WriteFloat(stream, vector.y);
    WriteFloat(stream, vector.z);
}

void WriteColor3i(DataStream* stream, Color3i color)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = color.colorArray[R_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[G_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[B_CHANNEL];
}

void WriteColor4i(DataStream* stream, Color4i color)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = color.colorArray[A_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[R_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[G_CHANNEL];
    stream->data[stream->pos++] = color.colorArray[B_CHANNEL];
}

void WriteColor3iv(DataStream* stream, uint8 r, uint8 g, uint8 b)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
}

void WriteColor4iv(DataStream* stream, uint8 a, uint8 r, uint8 g, uint8 b)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
    stream->data[stream->pos++] = a;
}
