#ifndef MAPSTRUCT_H
#define MAPSTRUCT_H

#include <Util/Queue.h>
#include <Util/Types.h>
#include <Util/Uthash.h>
#include <libmapvxl/libmapvxl.h>
#include <stddef.h>

typedef struct string_node
{
    char*               string;
    struct string_node *next, *prev;
} string_node_t;

typedef struct map
{
    uint8_t        map_count;
    string_node_t* current_map;
    // compressed map
    queue_t*       compressed_map;
    uint32_t       compressed_size;
    vector3i_t     result_line[50];
    size_t         map_size;
    mapvxl_t       map;
    string_node_t* map_list;
} map_t;

typedef struct map_node
{
    int            id;
    vector3i_t     pos;
    uint8_t        visited;
    UT_hash_handle hh;
} map_node_t;

#endif
