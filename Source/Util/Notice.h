#ifndef NOTICE_H
#define NOTICE_H

#include <Server/Structs/ServerStruct.h>

void send_server_notice(server_t* server, uint8_t player_id, uint8_t console, const char* message, ...);
void broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...);

#endif
