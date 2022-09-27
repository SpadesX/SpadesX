#include "ParseConvert.h"

#include "Structs.h"

#include <inttypes.h>
#include <stdio.h>

uint8_t format_str_to_ip(char* src, ip_t* dst)
{
    if (sscanf(src, "%hhu.%hhu.%hhu.%hhu/%hhu", &dst->ip[0], &dst->ip[1], &dst->ip[2], &dst->ip[3], &dst->cidr) == 5) {
        return 1;
    } else if (sscanf(src, "%hhu.%hhu.%hhu.%hhu", &dst->ip[0], &dst->ip[1], &dst->ip[2], &dst->ip[3]) == 4) {
        return 1;
    }
    return 0;
}

void format_ip_to_str(char* dst, ip_t src)
{
    if (src.cidr != 0) {
        snprintf(dst, 19, "%hhu.%hhu.%hhu.%hhu/%hhu", src.ip[0], src.ip[1], src.ip[2], src.ip[3], src.cidr);
        return;
    }
    snprintf(dst, 16, "%hhu.%hhu.%hhu.%hhu", src.ip[0], src.ip[1], src.ip[2], src.ip[3]);
}

void team_id_to_str(server_t* server, char* dst, int team)
{
    if (team == 255) {
        snprintf(dst, 15, "Spectator Team");
        return;
    }
    snprintf(dst, 11, "%s", server->protocol.name_team[team]);
}

uint8_t parse_player(char* s, uint8_t* id, char** end)
{
    uint8_t sLength;
    if (s[0] != '#' || (sLength = strlen(s)) < 2)
    { // TODO: look up a player by their nickname if the argument doesn't start with #
        return 0;
    }

    return parse_byte(s + 1, id, end);
}

uint8_t parse_byte(char* s, uint8_t* byte, char** end)
{
    char* _end;
    int   parsed = strtoimax(s, &_end, 10);
    if (!_end || _end == s || parsed < 0 || parsed > 255) {
        return 0;
    }

    *byte = parsed & 0xFF;
    if (end) {
        *end = _end;
    }
    return 1;
}

uint8_t parse_float(char* s, float* value, char** end)
{
    char*  _end;
    double parsed = strtod(s, &_end);
    if (!_end || _end == s) {
        return 0;
    }

    *value = (float) parsed;
    if (end) {
        *end = _end;
    }
    return 1;
}

uint8_t parse_ip(char* s, ip_t* ip, char** end)
{
    if (!parse_byte(s, &ip->ip[0], &s)) {
        return 0;
    }

    for (int i = 1; i < 4; i++) {
        if (*s++ != '.' || !parse_byte(s, &ip->ip[i], &s)) {
            return 0;
        }
    }

    if (*s == '/') { // We have a slash after the last number. That means we are dealing with cidr.
        if (!parse_byte(++s, &ip->cidr, &s) || ip->cidr > 32) {
            return 0;
        }
    } else {
        ip->cidr = 0;
    }

    if (end)
        *end = s;
    return 1;
}
