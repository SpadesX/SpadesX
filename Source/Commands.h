// Copyright DarkNeutrino 2021
#ifndef COMMANDS_H
#define COMMANDS_H

#include "Structs.h"

void handleCommands(Server* server, uint8 player, char* message);
void addCommands(Server* server);
void deleteCommands(Server* server);
#endif
