#pragma once
/* timeline.h
 *
 * RebbleOS
 *
 * Author: Carson Katri <me@carsonkatri.com>.
 */

#include "rebbleos.h"
#include "librebble.h"

void timeline_main(void);

typedef struct TimelineEventList {
    list_head event_list_head;
    uint16_t event_count;
} TimelineEventList;

typedef struct TimelineEvent {
    GBitmap *icon;
    const char *time;
    const char *name;
    const char *description;
    const char *app;
    GColor color;
    
    list_node node;
} TimelineEvent;

TimelineEvent* create_timeline_event(const char *time, const char *name, const char *description, const char *app, GBitmap *icon, GColor color);

#define TIMELINE_EVENT(name, time, description, app, icon, color) create_timeline_event(time, name, description, app, icon, color)

void timeline_add_event(TimelineEvent *event);
