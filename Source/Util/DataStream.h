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

typedef struct datastream {
    uint8_t* data;
    uint32_t length;
    uint32_t pos;
} datastream_t;

void     datastream_create(datastream_t* stream, uint32_t length);
void     datastream_free(datastream_t* stream);
uint32_t datastream_left(datastream_t* stream);
void     datastream_skip(datastream_t* stream, uint32_t skip);
uint8_t  datastream_read_u8(datastream_t* stream);
uint16_t datastream_read_u16(datastream_t* stream);
uint32_t datastream_read_u32(datastream_t* stream);
float    datastream_read_f(datastream_t* stream);
color_t  datastream_read_color_rgb(datastream_t* stream);
color_t  datastream_read_color_argb(datastream_t* stream);
void     datastream_read_array(datastream_t* stream, void* output, uint32_t length);
void     datastream_write_u8(datastream_t* stream, uint8_t value);
void     datastream_write_u16(datastream_t* stream, uint16_t value);
void     datastream_write_u32(datastream_t* stream, uint32_t value);
void     datastream_write_f(datastream_t* stream, float value);
void     datastream_write_vector3f(datastream_t* stream, vector3f_t vector);
void     datastream_write_color_rgb(datastream_t* stream, color_t color);
void     datastream_write_color_argb(datastream_t* stream, color_t color);
void     datastream_write_color_3u8(datastream_t* stream, uint8_t r, uint8_t g, uint8_t b);
void     datastream_write_color_4u8(datastream_t* stream, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     datastream_write_array(datastream_t* stream, const void* array, uint32_t length);

#endif /* DATASTREAM_H */
