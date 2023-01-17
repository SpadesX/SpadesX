#ifndef PARSECONVERT_H
#define PARSECONVERT_H

#include <Server/Structs/ServerStruct.h>
#include <Util/Types.h>
#include <enet/enet.h>
#include <string.h>

#define PARSE_VECTOR3F(argument, offset, dst)                                                                  \
    (parse_float(argument.argv[offset], dst.x, NULL) && parse_float(argument.argv[offset + 1], dst.y, NULL) && \
     parse_float(argument.argv[offset + 2], dst.z, NULL))

vector3i_t vec3f_to_vec3i(vector3f_t float_vec);
uint8_t    format_str_to_ip(char* src, ip_t* dst);
void       format_ip_to_str(char* dst, ip_t src);
void       team_id_to_str(server_t* server, char* dst, int team);
uint8_t    parse_player(char* s, uint8_t* player_id, char** end);
uint8_t    parse_byte(char* s, uint8_t* byte, char** end);
uint8_t    parse_float(char* s, float* value, char** end);
uint8_t    parse_ip(char* s, ip_t* ip, char** end);
void       copyBits(uint32_t* dst, uint32_t src, uint32_t endPos, uint32_t startPos);
void       swapIPStruct(ip_t* ip);
uint8_t    octets_in_range(ip_t start, ip_t end, ip_t host);
uint8_t    ip_in_range(ip_t host, ip_t banned, ip_t startOfRange, ip_t endOfRange);

#endif
