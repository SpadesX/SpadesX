// Copyright CircumScriptor and DarkNeutrino 2021
#include <Server/Structs/ServerStruct.h>
#include <Util/Compress.h>
#include <Util/Log.h>
#include <Util/Queue.h>
#include <Util/Utlist.h>
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
static int _compress_init(server_t* server, int level)
{
    if (g_compressor != NULL) {
        LOG_ERROR("Compressor is already initialized");
        return 1;
    } else {
        g_compressor = (z_stream*) malloc(sizeof(z_stream));
        if (g_compressor == NULL) {
            LOG_ERROR("Allocation of memory failed. EXITING");
            server->running = 0;
            return 0;
        }
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

queue_t* compress_queue(server_t* server, uint8_t* data, uint32_t length, uint32_t chunkSize)
{
    _compress_init(server, 5);
    if (g_compressor == NULL) {
        LOG_ERROR("Compressor is not initialized");
        return NULL;
    }

    queue_t* parent = NULL;
    queue_t* node;

    g_compressor->next_in  = (uint8_t*) data;
    g_compressor->avail_in = length;

    do {
        node        = (queue_t*) malloc(sizeof(*node));
        node->block = (uint8_t*) malloc(chunkSize);
        if (node == NULL || node->block == NULL) {
            LOG_ERROR("Allocation of memory failed. EXITING");
            server->running = 0;
            return 0;
        }
        g_compressor->avail_out = chunkSize;
        g_compressor->next_out  = node->block;
        if (deflate(g_compressor, Z_FINISH) < 0) {
            LOG_ERROR("Failed to compress data");
            return NULL;
        }
        node->length = chunkSize - g_compressor->avail_out;
        DL_APPEND(parent, node);
    } while (g_compressor->avail_out == 0);
    _compress_close();
    return parent;
}
