/* timeline.c
 *
 * The Rebble Timeline
 *
 * RebbleOS
 *
 * Author: Carson Katri <carson.katri@gmail.com>.
 */

#include "rebbleos.h"
#include "timeline.h"
#include "librebble.h"
#include "platform_res.h"

static const char* const s_months[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September","October", "November", "December"
};

static const char* const s_weekdays[] = {
	"S", "M", "T", "W", "T", "F", "S"
};

TimelineEvents *s_events;
int s_selected;
Layer *s_timeline;

int zeller(int day_of_month, int month, int year) {
  int m = month;
  if (m <= 1) {
    m += 13;
  } else {
    m += 1;
  }
  int y = year;
  if (month <= 1) {
    y--;
  }
  return (day_of_month + ((13 * (m + 1)) / 5) + y + (y / 4) - (y / 100) + (y / 400)) % 7;
}

int days_in_month(int month, int year) {
  static const int days[2][13] = {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };
  int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  return days[leap][month];
}

int s_month = 1;
int s_year = 2018;
int s_day_count = 31;
int s_day_offset = 0;

bool timeline = false;

static Window *s_main_window;
static MenuLayer *s_menu_layer;

TimelineEvents* timeline_events_create(uint16_t capacity) {
    TimelineEvents *result = (TimelineEvents *) app_calloc(1, sizeof(TimelineEvents));
    result->capacity = capacity;
    result->count = 0;
    if (capacity > 0)
        result->events = (TimelineEvent *) app_calloc(capacity, sizeof(TimelineEvent));

    return result;
}

void timeline_events_destroy(TimelineEvents *events) {
    if (!events)
        return;

    if (events->capacity > 0)
        app_free(events->events);

    app_free(events);
}

void timeline_events_add(TimelineEvents *events, TimelineEvent event) {
    if (events->capacity == events->count)
        return; // TODO: should we grow, or report error here?

    events->events[events->count++] = event;
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return timeline ? s_events->count : 31;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 0;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {

}

static n_GPathInfo triangle = {
  .num_points = 3,
  .points = (GPoint []) {{DISPLAY_COLS - 25, 5}, {DISPLAY_COLS - 25, 25}, {DISPLAY_COLS - 30, 15}}
};

static void timeline_update_proc(Layer *layer, GContext *ctx) {
  int row = (int) s_timeline->callback_data;
  TimelineEvent *event = &s_events->events[row];

  GRect frame = layer->bounds;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  if (timeline) {
    // TIMELINE UI
    graphics_context_set_fill_color(ctx, event->color);
    graphics_fill_rect(ctx, GRect(frame.size.w - 25, 0, 25, frame.size.h), 0, GCornerNone);
    n_gpath_fill(ctx, n_gpath_create(&triangle));

    graphics_draw_text(ctx, event->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(frame.origin.x, frame.origin.y, frame.size.w - 25, 36), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, event->description, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(frame.origin.x, frame.origin.y + 36, frame.size.w - 25, frame.size.h - 36), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    GBitmap *gbitmap = gbitmap_create_with_resource((int) event->image);
    graphics_draw_bitmap_in_rect(ctx, gbitmap, GRect(frame.size.w - 23, 5, 22, 22));
  } else {
    // CALENDAR UI
    graphics_draw_text(ctx, s_months[s_month - 1], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(frame.origin.x, frame.origin.y, frame.size.w, 36), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    // EACH WEEK
    int week_count = (int) s_day_count / 7;

    double day_offset = DISPLAY_COLS / 7;
    double week_offset = day_offset;

    for (int weekday = 0; weekday < 7; weekday++) {
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, s_weekdays[weekday], fonts_get_system_font(FONT_KEY_GOTHIC_09), GRect(day_offset * weekday, 24, day_offset, week_offset), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }

    for (int day = 0; day < s_day_count + s_day_offset; day++) {
      int week = (int) day / 7;

      // Check if this date has an event
      for (int i = 0; i < ((TimelineEvents *) s_events)->count; i++) {
        Date date = s_events->events[i].date;
        if ((int)date.date == (day - (s_day_offset - 1)) && (int)date.month == s_month && (int)date.year == s_year) {
          graphics_context_set_fill_color(ctx, s_events->events[i].color);
          graphics_context_set_text_color(ctx, GColorWhite);
          break;
        } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
          graphics_context_set_text_color(ctx, GColorBlack);
        }
      }
      graphics_context_set_stroke_color(ctx, day - s_day_offset == s_selected ? GColorBlack : GColorWhite);

      if (day < s_day_offset) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorLightGray);
      }

      GRect rect = GRect((day_offset * (day % 7)), (week_offset * week) + 36, day_offset, week_offset);
      graphics_fill_rect(ctx, rect, 0, GCornerNone);
      graphics_draw_rect(ctx, rect, 0, GCornerNone);

      char buf[] = "00000000000";
      snprintf(buf, sizeof(buf), "%d", (day > s_day_offset - 1) ? day + 1 - s_day_offset : days_in_month(s_month - 1 < 1 ? 12 : s_month - 1, s_year) - s_day_offset + day + 1);
      graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14), rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *index, void *data) {
  MenuIndex selected = menu_layer_get_selected_index(s_menu_layer);
  s_selected = selected.row;
  if (timeline) {
    // TIMELINE UI
    if (index->row == selected.row) {
      s_timeline->callback_data = (void *) selected.row;
    }
  } else {
    // CALENDAR UI
    s_timeline->callback_data = (void *) index->row;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *index, void *data) {
  // Show an info page or something...
  timeline = !timeline;
  layer_mark_dirty(s_timeline);
}

static void timeline_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_events = timeline_events_create(3);
  timeline_events_add(s_events, TimelineEvent("RWS Launch\n5:00pm", "Organizer: Katharine Berry\nInvitees: Rebblers\n\nEveryone will be there. You should too!", Date(1, 1, 2018), RESOURCE_ID_CLOCK, GColorRed));
  timeline_events_add(s_events, TimelineEvent("Meeting\n6:00pm", "Other content", Date(10, 1, 2018), RESOURCE_ID_SPANNER, GColorVividCerulean));
  timeline_events_add(s_events, TimelineEvent("RebbleOS\n7:00pm", "This is a ways away", Date(23, 1, 2018), RESOURCE_ID_SPEECH_BUBBLE, GColorIslamicGreen));

  s_day_count = days_in_month(s_month, s_year);
  printf("\n\nDAYS IN MONTH: %d\n\n", s_day_count);
  s_day_offset = zeller(0, s_month - 1, s_year);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  GRect frame = window_get_root_layer(s_main_window)->bounds;
  s_timeline = layer_create_with_data(frame, sizeof(int));
  layer_set_update_proc(s_timeline, timeline_update_proc);
  layer_add_child(menu_layer_get_layer(s_menu_layer), s_timeline);

  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void timeline_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

static void timeline_init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = timeline_window_load,
    .unload = timeline_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void timeline_deinit() {
  window_destroy(s_main_window);
}

void timeline_main(void) {
  timeline_init();
  app_event_loop();
  timeline_deinit();
}
