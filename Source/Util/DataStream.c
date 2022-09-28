// Copyright CircumScriptor and DarkNeutrino 2021
#include "DataStream.h"

#include <stdlib.h>
#include <string.h>

void stream_create(stream_t* stream, uint32_t length)
{
    stream->data   = malloc(length);
    stream->length = length;
    stream->pos    = 0;
}

void stream_free(stream_t* stream)
{
    if (stream->data != NULL) {
        free(stream->data);
    }
}

uint32_t stream_left(stream_t* stream)
{
    return (stream->pos < stream->length) ? stream->length - stream->pos : 0;
}

void stream_skip(stream_t* stream, uint32_t skip)
{
    stream->pos = (stream->pos + skip < stream->length) ? stream->pos + skip : stream->length;
}

uint8_t stream_read_u8(stream_t* stream)
{
    ACCESS_CHECK(stream, 1);
    return stream->data[stream->pos++];
}

uint16_t stream_read_u16(stream_t* stream)
{
    ACCESS_CHECK(stream, 2);
    uint16_t value = 0;
    value |= ((uint16_t) stream->data[stream->pos++]);
    value |= ((uint16_t) stream->data[stream->pos++]) << 8;
    return value;
}

uint32_t stream_read_u32(stream_t* stream)
{
    ACCESS_CHECK(stream, 4);
    uint32_t value = 0;
    value |= ((uint32_t) stream->data[stream->pos++]);
    value |= ((uint32_t) stream->data[stream->pos++]) << 8;
    value |= ((uint32_t) stream->data[stream->pos++]) << 16;
    value |= ((uint32_t) stream->data[stream->pos++]) << 24;
    return value;
}

float stream_read_f(stream_t* stream)
{
    union
    {
        float    f;
        uint32_t v;
    } u;
    u.v = stream_read_u32(stream);
    return u.f;
}

color_t stream_read_color_rgb(stream_t* stream)
{
    color_t color;
    if (stream->pos + 3 > stream->length) {
        color.raw = 0;
        return color;
    }
    color.a = 0xFF;
    color.r = stream->data[stream->pos++];
    color.g = stream->data[stream->pos++];
    color.b = stream->data[stream->pos++];
    return color;
}

color_t stream_read_color_argb(stream_t* stream)
{
    color_t color;
    if (stream->pos + 4 > stream->length) {
        color.raw = 0;
        return color;
    }
    color.a = stream->data[stream->pos++];
    color.r = stream->data[stream->pos++];
    color.g = stream->data[stream->pos++];
    color.b = stream->data[stream->pos++];
    return color;
}

void stream_read_array(stream_t* stream, void* output, uint32_t length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(output, stream->data + stream->pos, length); // Replace me
    stream->pos += length;
}

void stream_write_array(stream_t* stream, const void* array, uint32_t length)
{
    ACCESS_CHECK_N(stream, length);
    memcpy(stream->data + stream->pos, array, length); // Replace me
    stream->pos += length;
}

void stream_write_u8(stream_t* stream, uint8_t value)
{
    ACCESS_CHECK_N(stream, 1);
    stream->data[stream->pos++] = value;
}

void stream_write_u16(stream_t* stream, uint16_t value)
{
    ACCESS_CHECK_N(stream, 2);
    stream->data[stream->pos++] = (uint8_t) value;
    stream->data[stream->pos++] = (uint8_t) (value >> 8);
}

void stream_write_u32(stream_t* stream, uint32_t value)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = (uint8_t) value;
    stream->data[stream->pos++] = (uint8_t) (value >> 8);
    stream->data[stream->pos++] = (uint8_t) (value >> 16);
    stream->data[stream->pos++] = (uint8_t) (value >> 24);
}

void stream_write_f(stream_t* stream, float value)
{
    union
    {
        float    f;
        uint32_t v;
    } u;
    u.f = value;
    stream_write_u32(stream, u.v);
}

void stream_write_vector3f(stream_t* stream, vector3f_t vector)
{
    stream_write_f(stream, vector.x);
    stream_write_f(stream, vector.y);
    stream_write_f(stream, vector.z);
}

void stream_write_color_rgb(stream_t* stream, color_t color)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = color.b;
    stream->data[stream->pos++] = color.g;
    stream->data[stream->pos++] = color.r;
}

void stream_write_color_argb(stream_t* stream, color_t color)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = color.b;
    stream->data[stream->pos++] = color.g;
    stream->data[stream->pos++] = color.r;
    stream->data[stream->pos++] = color.a;
}

void stream_write_color_3u8(stream_t* stream, uint8_t r, uint8_t g, uint8_t b)
{
    ACCESS_CHECK_N(stream, 3);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
}

void stream_write_color_4u8(stream_t* stream, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    ACCESS_CHECK_N(stream, 4);
    stream->data[stream->pos++] = b;
    stream->data[stream->pos++] = g;
    stream->data[stream->pos++] = r;
    stream->data[stream->pos++] = a;
}
