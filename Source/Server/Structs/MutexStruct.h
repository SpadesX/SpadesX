#ifndef MUTEXSTRUCT_H
#define MUTEXSTRUCT_H

#include <bits/pthreadtypes.h>
typedef struct server_mutex
{
    pthread_mutex_t players;
    pthread_mutex_t protocol;
    pthread_mutex_t physics;
    pthread_mutex_t master;
    pthread_mutex_t packets;
    pthread_mutex_t map;
    pthread_mutex_t timers;
    pthread_mutex_t commands;
    pthread_mutex_t messages;
    pthread_mutex_t passwords;
    pthread_mutex_t flags;
    // This is basically used as an legacy or when we need to block literally everything
    pthread_mutex_t global_server;
} server_mutex_t;

#endif
