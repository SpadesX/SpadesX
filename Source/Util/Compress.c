// Copyright CircumScriptor and DarkNeutrino 2021
#include "Compress.h"

#include "Log.h"
#include "Queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

static z_stream* g_compressor = NULL;

/**
 * @brief Initialize compressor library (global compressor)
 *
 * @param level Level of compression
 * @return 0 on success
 */
static int _compress_init(int level)
{
    if (g_compressor != NULL) {
        LOG_ERROR("Compressor is already initialized");
        return 1;
    } else {
        g_compressor = (z_stream*) malloc(sizeof(z_stream));
    }

    g_compressor->zalloc = Z_NULL;
    g_compressor->zfree  = Z_NULL;
    g_compressor->opaque = Z_NULL;

    if (deflateInit(g_compressor, level) < 0) {
        LOG_ERROR("Failed to initialize compressor");
        return 1;
    }
    return 0;
}

/**
 * @brief Close compressor library (global compressor)
 *
 * @return 0 on success
 */
static int _compress_close(void)
{
    if (g_compressor == NULL) {
        LOG_ERROR("Compressor is not initialized");
        return 1;
    }

    if (deflateEnd(g_compressor) != 0) {
        LOG_ERROR("Failed to close compressor");
        return 1;
    }

    free(g_compressor);
    g_compressor = NULL;
    return 0;
}

queue_t* compress_queue(uint8_t* data, uint32_t length, uint32_t chunkSize)
{
    _compress_init(5);
    if (g_compressor == NULL) {
        LOG_ERROR("Compressor is not initialized");
        return NULL;
    }

    queue_t* first = NULL;
    queue_t* node  = NULL;

    g_compressor->next_in  = (uint8_t*) data;
    g_compressor->avail_in = length;

    do {
        if (first == NULL) {
            first = queue_push(NULL, chunkSize);
            node  = first;
        } else {
            node = queue_push(node, chunkSize);
        }

        g_compressor->avail_out = chunkSize;
        g_compressor->next_out  = node->block;
        if (deflate(g_compressor, Z_FINISH) < 0) {
            LOG_ERROR("Failed to compress data");
            return NULL;
        }
        node->length = chunkSize - g_compressor->avail_out;
    } while (g_compressor->avail_out == 0);
    _compress_close();
    return first;
}
