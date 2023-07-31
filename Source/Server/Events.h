// Copyright SpadesX team 2023
#ifndef EVENTS_H
#define EVENTS_H

#include <Server/Structs/PlayerStruct.h>
#include <Util/Alloc.h>
#include <Util/Log.h>
#include <Util/Utlist.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// return values for a callback:
enum {
    EVENT_CONTINUE, // allow to run the next callback
    EVENT_BREAK     // stop processing callbacks
};

// where to insert the callback:
enum { EVENT_FIRST, EVENT_LAST };

// This macro contains forward declarations of the event functions and should be called in a header.
//
// Arguments:
// - event - event name. Should be a valid variable name in C
// - ... - additional info for the callbacks (example: const char* Name, int Number, void* Data)
//
// Defined types:
// - int [event]Callback([callbackArguments]) - describes a callback function
// - struct [event]Listener - an element of the callback linked list
//
// Defined global variables:
// - struct [event]Listener *g_[event]Listeners - a linked list of callbacks for given event
//
// Defined functions:
// - void [event]_subscribe([event]Callback callback, int position) - adds a callback to the list. Position is either
// EVENT_FIRST or EVENT_LAST
// - void [event]_unsubscribe([event]Callback callback) - find given callback in the list and remove it if it's present
// - void [event]_run(...) - called when the event had occured. Calls every callback until either callback returns
// EVENT_BREAK or there will be no more callbacks in the list
#define EVENT(event, ...)                                                                  \
    typedef int (*event##Callback)(server_t * server, __VA_ARGS__);                        \
    typedef struct event##Listener_t                                                       \
    {                                                                                      \
        event##Callback           callback;                                                \
        struct event##Listener_t* next;                                                    \
    } event##Listener_t;                                                                   \
                                                                                           \
    void on_##event##_subscribe(server_t* server, event##Callback callback, int position); \
    void on_##event##_unsubscribe(server_t* server, event##Callback callback);             \
    void on_##event##_run(server_t* server, __VA_ARGS__);

// This macro defines the functions and a global list of callbacks and should be calleed in a source file.
// Arguments:
// - event - same as EVENT
// - argumentNames - same as ... in EVENT but in brackets and without the type names, pointer signs etc (example: (Name,
// Number, Data))
// - ... - same as EVENT
#define EVENT_DEFINITION(event, argumentNames, ...)                                       \
    void on_##event##_subscribe(server_t* server, event##Callback callback, int position) \
    {                                                                                     \
        event##Listener_t* el;                                                            \
        el           = spadesx_malloc(sizeof *el);                                        \
        el->callback = callback;                                                          \
        if (position == EVENT_LAST)                                                       \
            LL_APPEND(server->event_handlers.event, el);                                  \
        else                                                                              \
            LL_PREPEND(server->event_handlers.event, el);                                 \
    }                                                                                     \
                                                                                          \
    void on_##event##_unsubscribe(server_t* server, event##Callback callback)             \
    {                                                                                     \
        event##Listener_t* el;                                                            \
        LL_SEARCH_SCALAR(server->event_handlers.event, el, callback, callback);           \
        if (!el)                                                                          \
            return;                                                                       \
        LL_DELETE(server->event_handlers.event, el);                                      \
        free(el);                                                                         \
    }                                                                                     \
                                                                                          \
    void on_##event##_run(server_t* server, __VA_ARGS__)                                  \
    {                                                                                     \
        event##Listener_t* el;                                                            \
        el = server->event_handlers.event;                                                \
        while (el && el->callback argumentNames != EVENT_BREAK)                           \
            el = el->next;                                                                \
    }

// Happens when a player disconnects.
// Arguments: player struct pointer
EVENT(player_disconnect, player_t* player)

#endif
