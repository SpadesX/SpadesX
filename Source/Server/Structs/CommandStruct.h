#ifndef COMMANDSTRUCT_H
#define COMMANDSTRUCT_H

#include <Util/Types.h>
#include <Util/Uthash.h>

typedef struct player player_t;
typedef struct server server_t;

typedef struct permissions
{
    char         access_level[32];
    const char** access_password;
    uint8_t      perm_level_offset;
} permissions_t;

typedef struct command_args
{
    player_t* player;
    server_t* server;
    uint8_t   console;
    uint32_t  permissions;
    uint32_t  argc;
    char*     argv[32]; // 32 roles commands should be more than enough for anyone
} command_args_t;

typedef struct command
{
    char    id[30];
    uint8_t parse_args; // Should we even bother parsing it? Would be useful for commands whose one and only
                        // argument is a message, or which don't even have any arguments
    void (*execute)(void* p_server, command_args_t arguments);
    uint32_t        permissions; // 32 roles should be more then enough for anyone
    char            description[1024];
    UT_hash_handle  hh;
    struct command* next;
} command_t;

// Command but with removed hash map and linked list stuff for easy managment
typedef struct command_manager
{
    char    id[30];
    uint8_t parse_args; // Should we even bother parsing it? Would be useful for commands whose one and only
                        // argument is a message, or which don't even have any arguments
    void (*command)(void* p_server, command_args_t arguments);
    uint32_t permissions; // 32 roles should be more then enough for anyone
    char     description[1024];
} command_manager_t;

#endif
