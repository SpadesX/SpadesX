#ifndef PARSECONVERT_H
#define PARSECONVERT_H

#include "Structs.h"

#include <Types.h>

#include <string.h>
#include <enet/enet.h>

#define PARSE_VECTOR3F(argument, offset, dest) (parseFloat(argument.argv[offset], dest.x, NULL) && parseFloat(argument.argv[offset + 1], dest.y, NULL) && parseFloat(argument.argv[offset + 2], dest.z, NULL))

void formatIPToString(char *dest, IPStruct src);
uint8 parsePlayer(char* s, uint8* id, char** end);
uint8 parseByte(char* s, uint8* byte, char** end);
uint8 parseFloat(char* s, float* value, char** end);
uint8 parseIP(char* s, IPStruct* ip, char **end);

#endif
