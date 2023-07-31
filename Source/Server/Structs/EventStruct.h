#ifndef EVENTSTRUCT_H
#define EVENTSTRUCT_H

#include <Server/Events.h>

#define EVENT_HANDLERS_LIST(event) event##Listener_t* event

typedef struct event_handlers
{
    EVENT_HANDLERS_LIST(player_disconnect);
} event_handlers_t;

#endif
