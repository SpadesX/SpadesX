#include "Structs.h"

#include <inttypes.h>
#include <stdio.h>

uint8 formatStringToIP(char* src, IPStruct* dest)
{
    if (sscanf(src,
               "%hhu.%hhu.%hhu.%hhu/%hhu",
               &dest->Union.ip[0],
               &dest->Union.ip[1],
               &dest->Union.ip[2],
               &dest->Union.ip[3],
               &dest->CIDR) == 5)
    {
        return 1;
    } else if (sscanf(src,
                      "%hhu.%hhu.%hhu.%hhu",
                      &dest->Union.ip[0],
                      &dest->Union.ip[1],
                      &dest->Union.ip[2],
                      &dest->Union.ip[3]) == 4)
    {
        return 1;
    }
    return 0;
}

void formatIPToString(char* dest, IPStruct src)
{
    if (src.CIDR != 0) {
        snprintf(dest,
                 19,
                 "%hhu.%hhu.%hhu.%hhu/%hhu",
                 src.Union.ip[0],
                 src.Union.ip[1],
                 src.Union.ip[2],
                 src.Union.ip[3],
                 src.CIDR);
        return;
    }
    snprintf(dest, 16, "%hhu.%hhu.%hhu.%hhu", src.Union.ip[0], src.Union.ip[1], src.Union.ip[2], src.Union.ip[3]);
}

void teamIDToString(Server* server, char* dest, int team)
{
    if (team == 255) {
        snprintf(dest, 15, "Spectator Team");
        return;
    }
    snprintf(dest, 11, "%s", server->protocol.nameTeam[team]);
}

uint8 parsePlayer(char* s, uint8* id, char** end)
{
    uint8 sLength;
    char* _end;
    if (s[0] != '#' || (sLength = strlen(s)) < 2)
    { // TODO: look up a player by their nickname if the argument doesn't start with #
        return 0;
    }

    *id = strtoimax(s + 1, &_end, 10);
    if (!_end || _end == s) {
        return 0;
    }

    if (end) {
        *end = _end;
    }
    return 1;
}

uint8 parseByte(char* s, uint8* byte, char** end)
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

uint8 parseFloat(char* s, float* value, char** end)
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

uint8 parseIP(char* s, IPStruct* ip, char** end)
{
    if (!parseByte(s, &ip->Union.ip[0], &s)) {
        return 0;
    }

    for (int i = 1; i < 4; i++) {
        if (*s++ != '.' || !parseByte(s, &ip->Union.ip[i], &s)) {
            return 0;
        }
    }

    if (*s == '/') { // We have a slash after the last number. That means we are dealing with CIDR.
        if (!parseByte(++s, &ip->CIDR, &s) || ip->CIDR > 32) {
            return 0;
        }
    } else {
        ip->CIDR = 0;
    }

    if (end)
        *end = s;
    return 1;
}
