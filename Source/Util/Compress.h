//Copyright CircumScriptor and DarkNeutrino 2021
#ifndef COMPRESS_H
#define COMPRESS_H

#include "Queue.h"
#include "Types.h"

#ifndef DEFAULT_COMPRESSOR_CHUNK_SIZE
    #define DEFAULT_COMPRESSOR_CHUNK_SIZE 8192
#endif /* DEFAULT_COMPRESSOR_CHUNK_SIZE */

/**
 * @brief Initialize compressor library (global compressor)
 *
 * @param level Level of compression
 * @return 0 on success
 */
int InitCompressor(int level);

/**
 * @brief Close compressor library (global compressor)
 *
 * @return 0 on success
 */
int CloseCompressor();

/**
 * @brief Compress data (using global compressor)
 *
 * @param data Data to be compressed
 * @param length Length of data in bytes
 * @param chunkSize Size of one block of compressed data
 * @return Blocks of compressed data (queue) or null
 */
Queue* CompressData(uint8* data, uint32 length, uint32 chunkSize);

#endif // COMPRESS_H
