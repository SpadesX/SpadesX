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
uint32 DataLeft(DataStream* stream);
void StreamSkip(DataStream* stream, uint32 skip);
uint8 ReadByte(DataStream* stream);
uint16 ReadShort(DataStream* stream);
uint32 ReadInt(DataStream* stream);
float ReadFloat(DataStream* stream);
void ReadColor3i(DataStream* stream, Color3i color);
void ReadColor4i(DataStream* stream, Color4i color);
void ReadArray(DataStream* stream, void* output, uint32 length);
void WriteByte(DataStream* stream, uint8 value);
void WriteShort(DataStream* stream, uint16 value);
void WriteInt(DataStream* stream, uint32 value);
void WriteFloat(DataStream* stream, float value);
void WriteVector3f(DataStream* stream, Vector3f vector);
void WriteColor3i(DataStream* stream, Color3i color);
void WriteColor4i(DataStream* stream, Color4i color);
void WriteColor3iv(DataStream* stream, uint8 r, uint8 g, uint8 b);
void WriteColor4iv(DataStream* stream, uint8 a, uint8 r, uint8 g, uint8 b);
void WriteArray(DataStream* stream, const void* array, uint32 length);

#endif /* DATASTREAM_H */
