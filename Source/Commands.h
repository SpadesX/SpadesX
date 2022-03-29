// Copyright DarkNeutrino 2021
#ifndef COMMANDS_H
#define COMMANDS_H

#include "Structs.h"

#define FORMAT_IP(dest, src) snprintf(dest, 16, "%hhu.%hhu.%hhu.%hhu", src.Union.ip[0], src.Union.ip[1], src.Union.ip[2], src.Union.ip[3])
#define PARSE_VECTOR3F(argument, offset, dest) (parseFloat(argument.argv[offset], dest.x, NULL) && parseFloat(argument.argv[offset + 1], dest.y, NULL) && parseFloat(argument.argv[offset + 2], dest.z, NULL))

void handleCommands(Server* server, uint8 player, char* message);
void populateCommands(Server* server);
void freeCommands(Server* server);
void createCommand(Server* server,
                   uint8 parseArguments,
                   void (*command)(void* serverP, CommandArguments arguments),
                   char   id[30],
                   char   commandDesc[1024],
                   uint32 permLevel);
void deleteCommand(Server* server, Command* command);

uint8 parsePlayer(char* s, uint8* id, char** end);
uint8 parseByte(char* s, uint8* byte, char** end);
uint8 parseFloat(char* s, float* value, char** end);
uint8 parseIP(char* s, IPStruct* ip);

#endif
