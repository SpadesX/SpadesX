#ifndef PACKETCHECKS_H
#define PACKETCHECKS_H

#include <Server/Structs/ServerStruct.h>

uint8_t send_packet_except_sender(server_t* server, ENetPacket* packet, uint8_t player_id);
uint8_t send_packet_except_sender_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id);
uint8_t send_packet_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id);

#endif
