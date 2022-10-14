// Copyright DarkNeutrino 2021
#ifndef SERVER_H
#define SERVER_H

#define VERSION \
    "Beta 2.0"  \
    " Compiled on " __DATE__ " " __TIME__

#include <Server/Structs/ServerStruct.h>
#include <Server/Structs/StartStruct.h>
#include <Util/Enums.h>
#include <Util/Line.h>
#include <Util/Queue.h>
#include <Util/Types.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <pthread.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

extern pthread_mutex_t server_lock;

server_t* get_server(void);

void stop_server(void);

void server_start(server_args args);

void server_reset(server_t* server);

#endif /* SERVER_H */
