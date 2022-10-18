// Copyright SpadesX team 2022
#ifndef EVENTS_H
#define EVENTS_H

#include <Server/Structs/PlayerStruct.h>
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
#define EVENT(event, ...)                                           \
    typedef int (*event##Callback)(__VA_ARGS__);                    \
    typedef struct event##Listener                                  \
    {                                                               \
        int (*callback)(__VA_ARGS__);                               \
        struct event##Listener* next;                               \
    } event##Listener;                                              \
                                                                    \
    extern struct event##Listener* g_##event##Listeners;            \
                                                                    \
    void event##_subscribe(event##Callback callback, int position); \
    void event##_unsubscribe(event##Callback callback);             \
    void event##_run(__VA_ARGS__);

// This macro defines the functions and a global list of callbacks and should be calleed in a source file.
// Arguments:
// - event - same as EVENT
// - argumentNames - same as ... in EVENT but in brackets and without the type names, pointer signs etc (example: (Name,
// Number, Data))
// - ... - same as EVENT
#define EVENT_DEFINITION(event, argumentNames, ...)                     \
    struct event##Listener* g_##event##Listeners = NULL;                \
                                                                        \
    void event##_subscribe(event##Callback callback, int position)      \
    {                                                                   \
        struct event##Listener* el;                                     \
        el = malloc(sizeof *el);                                        \
        if (!el) {                                                      \
            LOG_ERROR("Out of memory. Oh well.");                       \
            return;                                                     \
        }                                                               \
        el->callback = callback;                                        \
        if (position == EVENT_LAST)                                     \
            LL_APPEND(g_##event##Listeners, el);                        \
        else                                                            \
            LL_PREPEND(g_##event##Listeners, el);                       \
    }                                                                   \
                                                                        \
    void event##_unsubscribe(event##Callback callback)                  \
    {                                                                   \
        struct event##Listener* el;                                     \
        LL_SEARCH_SCALAR(g_##event##Listeners, el, callback, callback); \
        if (!el)                                                        \
            return;                                                     \
        LL_DELETE(g_##event##Listeners, el);                            \
        free(el);                                                       \
    }                                                                   \
                                                                        \
    void event##_run(__VA_ARGS__)                                       \
    {                                                                   \
        struct event##Listener* el;                                     \
        el = g_##event##Listeners;                                      \
        while (el && el->callback argumentNames != EVENT_BREAK)         \
            el = el->next;                                              \
    }

// Happens when a player disconnects.
// Arguments: player struct pointer
EVENT(player_disconnect, player_t* player)

#endif
