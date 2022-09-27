#ifndef PARSECONVERT_H
#define PARSECONVERT_H

#include "Structs.h"
#include "Util/Types.h"

#include <enet/enet.h>
#include <string.h>

#define PARSE_VECTOR3F(argument, offset, dst)                                                                  \
    (parse_float(argument.argv[offset], dst.x, NULL) && parse_float(argument.argv[offset + 1], dst.y, NULL) && \
     parse_float(argument.argv[offset + 2], dst.z, NULL))

uint8_t format_str_to_ip(char* src, ip_t* dst);
void    format_ip_to_str(char* dst, ip_t src);
void    team_id_to_str(server_t* server, char* dst, int team);
uint8_t parse_player(char* s, uint8_t* id, char** end);
uint8_t parse_byte(char* s, uint8_t* byte, char** end);
uint8_t parse_float(char* s, float* value, char** end);
uint8_t parse_ip(char* s, ip_t* ip, char** end);

#endif
