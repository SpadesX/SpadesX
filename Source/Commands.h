// Copyright DarkNeutrino 2021
#ifndef COMMANDS_H
#define COMMANDS_H

#include "Structs.h"

void handleCommands(Server* server, uint8 player, char* message);
void populateCommands(Server* server);
void freeCommands(Server* server);
void createCommand(Server* server,
                   uint8   parseArguments,
                   void (*command)(void* serverP, CommandArguments arguments),
                   char   id[30],
                   char   commandDesc[1024],
                   uint32 permLevel);
void deleteCommand(Server* server, Command* command);
#endif
