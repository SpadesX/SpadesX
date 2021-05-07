//Copyright CircumScriptor and DarkNeutrino 2021
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

void ReadArray(DataStream* stream, void* output, uint32 length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(output, stream->data + stream->pos, length);
    stream->pos += length;
}

void WriteArray(DataStream* stream, const void* array, uint32 length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(stream->data + stream->pos, array, length);
    stream->pos += length;
}
