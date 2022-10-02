#ifndef STAFF_H
#define STAFF_H

#include <Server/Structs/ServerStruct.h>

uint8_t is_staff(server_t* server, uint8_t player_id);
void    send_message_to_staff(server_t* server, const char* format, ...);

#endif
