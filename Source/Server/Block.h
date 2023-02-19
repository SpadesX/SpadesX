#include <Server/Structs/PacketStruct.h>

#ifndef BLOCK_H
#define BLOCK_H

void handle_block_action(server_t*  server,
                         player_t*  player,
                         uint8_t    action_type,
                         vector3i_t vector_block,
                         vector3f_t vectorf_block,
                         vector3f_t player_vector,
                         uint32_t   X,
                         uint32_t   Y,
                         uint32_t   Z);

#endif
