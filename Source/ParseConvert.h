#ifndef PARSECONVERT_H
#define PARSECONVERT_H

#include "Structs.h"
#include "Util/Types.h"

#include <enet/enet.h>
#include <string.h>

#define PARSE_VECTOR3F(argument, offset, dest)                                                                 \
    (parseFloat(argument.argv[offset], dest.x, NULL) && parseFloat(argument.argv[offset + 1], dest.y, NULL) && \
     parseFloat(argument.argv[offset + 2], dest.z, NULL))

uint8 formatStringToIP(char* src, IPStruct *dest);
void  formatIPToString(char* dest, IPStruct src);
uint8 parsePlayer(char* s, uint8* id, char** end);
uint8 parseByte(char* s, uint8* byte, char** end);
uint8 parseFloat(char* s, float* value, char** end);
uint8 parseIP(char* s, IPStruct* ip, char** end);

#endif
