#pragma once
/* timeline.h
 *
 * The Rebble Timeline
 *
 * RebbleOS
 *
 * Author: Carson Katri <carson.katri@gmail.com>.
 */

#include "display.h"
#include "backlight.h"
#include "rebbleos.h"
#include "menu.h"

struct TimelineEvent;
struct TimelineEvents;
struct Date;

typedef struct Date {
  int *date;
  int *month;
  int *year;
} Date;

#define Date(date, month, year) ((Date) { (int *) date, (int *) month, (int *) year })

typedef struct TimelineEvent {
  char *name;
  char *description;
  Date date;
  uint16_t *image;
  GColor color;
} TimelineEvent;

#define TimelineEvent(name, description, date, image, color) ((TimelineEvent) { name, description, date, (int *) image, color })

typedef struct TimelineEvents {
  TimelineEvent *events;
  uint16_t capacity;
  uint16_t count;
} TimelineEvents;

TimelineEvents* timeline_events_create(uint16_t capacity);
void timeline_events_destroy(TimelineEvents *events);
void timeline_events_add(TimelineEvents *events, TimelineEvent event);

static MenuItems* timeline_event_selected(const MenuItem *item);

void timeline_main(void);
