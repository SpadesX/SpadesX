// Copyright DarkNeutrino 2021
#ifndef SERVER_H
#define SERVER_H

#include "../Extern/libmapvxl/libmapvxl.h"
#include "Protocol.h"
#include "Util/Enums.h"
#include "Util/Line.h"
#include "Util/Queue.h"
#include "Util/Types.h"

#include <enet/enet.h>
#include <pthread.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

extern pthread_mutex_t server_lock;

server_t* get_server(void);

void server_start(uint16_t       port,
                  uint32_t       connections,
                  uint32_t       channels,
                  uint32_t       in_bandwidth,
                  uint32_t       out_bandwidth,
                  uint8_t        master,
                  string_node_t* map_list,
                  uint8_t        map_count,
                  string_node_t* welcome_message_list,
                  uint8_t        welcome_message_list_length,
                  string_node_t* periodic_message_list,
                  uint8_t        periodic_message_list_length,
                  uint8_t*       periodic_delays,
                  const char*    manager_password,
                  const char*    admin_password,
                  const char*    mod_passowrd,
                  const char*    guard_password,
                  const char*    trusted_password,
                  const char*    server_name,
                  const char*    team1_name,
                  const char*    team2_name,
                  uint8_t*       team1_color,
                  uint8_t*       team2_color,
                  uint8_t        gamemode);

void server_reset(server_t* server);

#endif /* SERVER_H */
