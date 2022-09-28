// Copyright DarkNeutrino 2021
#ifndef COMMANDS_H
#define COMMANDS_H

#include "Structs.h"
#include "../Extern/libmapvxl/libmapvxl.h"

void command_handle(server_t* server, uint8_t player, char* message, uint8_t console);
void command_populate_all(server_t* server);
void command_free_all(server_t* server);
void command_create(server_t* server,
                    uint8_t   parse_args,
                    void (*command)(void* server, command_args_t arguments),
                    char     id[30],
                    char     description[1024],
                    uint32_t permissions);
void command_free(server_t* server, command_t* command);

#endif
