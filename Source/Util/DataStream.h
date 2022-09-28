// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef stream_H
#define stream_H

#include "Enums.h"
#include "Types.h"

typedef struct _ENetPacket ENetPacket;

#define ACCESS_CHECK(stream, size)                \
    if (stream->offset + size > stream->length) { \
        return 0;                                 \
    }

#define ACCESS_CHECK_N(stream, size)              \
    if (stream->offset + size > stream->length) { \
        return;                                   \
    }

typedef struct stream
{
    uint8_t* data;
    uint32_t length;
    uint32_t offset;
} stream_t;

void     stream_from_enet_packet(stream_t* stream, ENetPacket* packet);
uint32_t stream_left(stream_t* stream);
void     stream_skip(stream_t* stream, uint32_t skip);
uint8_t  stream_read_u8(stream_t* stream);
uint16_t stream_read_u16(stream_t* stream);
uint32_t stream_read_u32(stream_t* stream);
float    stream_read_f(stream_t* stream);
color_t  stream_read_color_rgb(stream_t* stream);
color_t  stream_read_color_argb(stream_t* stream);
void     stream_read_array(stream_t* stream, void* output, uint32_t length);
void     stream_write_u8(stream_t* stream, uint8_t value);
void     stream_write_u16(stream_t* stream, uint16_t value);
void     stream_write_u32(stream_t* stream, uint32_t value);
void     stream_write_f(stream_t* stream, float value);
void     stream_write_vector3f(stream_t* stream, vector3f_t vector);
void     stream_write_color_rgb(stream_t* stream, color_t color);
void     stream_write_color_argb(stream_t* stream, color_t color);
void     stream_write_color_3u8(stream_t* stream, uint8_t r, uint8_t g, uint8_t b);
void     stream_write_color_4u8(stream_t* stream, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     stream_write_array(stream_t* stream, const void* array, uint32_t length);

#endif /* stream_H */
