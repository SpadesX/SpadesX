// Copyright CircumScriptor and DarkNeutrino 2021
#ifndef COMPRESS_H
#define COMPRESS_H

#include "Queue.h"
#include "Types.h"

#ifndef DEFAULT_COMPRESS_CHUNK_SIZE
    #define DEFAULT_COMPRESS_CHUNK_SIZE 8192
#endif /* DEFAULT_COMPRESS_CHUNK_SIZE */

/**
 * @brief Compress data (using global compressor)
 *
 * @param data Data to be compressed
 * @param length Length of data in bytes
 * @param chunkSize Size of one block of compressed data
 * @return Blocks of compressed data (queue) or null
 */
queue_t* compress_queue(uint8_t* data, uint32_t length, uint32_t chunkSize);

#endif // COMPRESS_H
