#include "Util/Types.h"

#include <Server/ParseConvert.h>
#include <Server/Structs/ServerStruct.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>

void copyBits(uint32_t* dst, uint32_t src, uint32_t endPos, uint32_t startPos)
{
    uint32_t numBitsToCopy = endPos - startPos + 1;
    uint32_t numBitsInInt  = sizeof(uint32_t) * 8;
    uint32_t zeroMask;
    uint32_t onesMask = ~((uint32_t) 0);

    onesMask = onesMask >> (numBitsInInt - numBitsToCopy);
    onesMask = onesMask << startPos;
    zeroMask = ~onesMask;
    *dst     = *dst & zeroMask;
    src      = src & onesMask;
    *dst     = *dst | src;
}

vector3i_t vec3f_to_vec3i(vector3f_t float_vec)
{
    // We cannot just type cast them as that does weird things between -1 to 0
    vector3i_t int_vec = {floor(float_vec.x), floor(float_vec.y), floor(float_vec.z)};
    return int_vec;
}

void swapIPStruct(ip_t* ip)
{
    uint8_t tmp;
    tmp       = ip->ip[0];
    ip->ip[0] = ip->ip[3];
    ip->ip[3] = tmp;
    tmp       = ip->ip[1];
    ip->ip[1] = ip->ip[2];
    ip->ip[2] = tmp;
}

uint8_t octets_in_range(ip_t start, ip_t end, ip_t host)
{
    swapIPStruct(&start);
    swapIPStruct(&end);
    swapIPStruct(&host);
    if (host.ip32 >= start.ip32 && host.ip32 <= end.ip32) {
        return 1;
        swapIPStruct(&start);
        swapIPStruct(&end);
        swapIPStruct(&host);
    }
    swapIPStruct(&start);
    swapIPStruct(&end);
    swapIPStruct(&host);
    return 0;
}

uint8_t ip_in_range(ip_t host, ip_t banned, ip_t startOfRange, ip_t endOfRange)
{
    uint32_t max = UINT32_MAX;
    ip_t     startRange;
    ip_t     endRange;
    startRange.cidr = 24;
    endRange.cidr   = 24;
    startRange.ip32 = 0;
    endRange.ip32   = 0;
    startRange.ip32 = startOfRange.ip32;
    endRange.ip32   = endOfRange.ip32;
    if (banned.cidr > 0 && banned.cidr < 32) {
        uint32_t subMax = 0;
        copyBits(&subMax, max, 31 - banned.cidr, 0);
        swapIPStruct(&banned);
        uint32_t ourRange = 0;
        copyBits(&ourRange, banned.ip32, 32 - banned.cidr, 0);
        swapIPStruct(&banned);
        uint32_t times = ourRange / (subMax + 1);
        uint32_t start = times * (subMax + 1);
        ip_t     startIP;
        ip_t     endIP;
        startIP.ip32 = banned.ip32;
        endIP.ip32   = banned.ip32;
        startIP.cidr = 0;
        endIP.cidr   = 0;
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        uint32_t end = ((times + 1) * subMax);
        end |= 1;
        copyBits(&startIP.ip32, start, 32 - banned.cidr, 0);
        copyBits(&endIP.ip32, end, 32 - banned.cidr, 0);
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        if (octets_in_range(startIP, endIP, host)) {
            return 1;
        }
    } else if (octets_in_range(startRange, endRange, host)) {
        return 1;
    } else if (host.ip32 == banned.ip32) {
        return 1;
    }
    return 0;
}

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

uint8_t parse_player(char* s, uint8_t* player_id, char** end)
{
    uint8_t sLength;
    if (s[0] != '#' || (sLength = strlen(s)) < 2)
    { // TODO: look up a player by their nickname if the argument doesn't start with #
        return 0;
    }

    return parse_byte(s + 1, player_id, end);
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
