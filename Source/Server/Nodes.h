// Copyright DarkNeutrino 2021
#ifndef NODES_H
#define NODES_H

#include <Server/Structs/ServerStruct.h>

vector3i_t* get_neighbours(vector3i_t pos);
uint8_t     check_node(server_t* server, vector3i_t position);

#endif
