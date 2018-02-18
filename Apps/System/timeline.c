/* timeline.c
 * Timeline app (launched from the watchface or the calendar app)
 *
 * Author: Carson Katri <me@carsonkatri.com>
 */

#include "librebble.h"
#include "graphics_wrapper.h"
#include "menu_layer.h"
#include "platform_res.h"
#include "notification_layer.h"
#include "timeline.h"

static MenuLayer* s_menu_layer;
static Window* timeline_window;

static TimelineEventList event_list;

static TimelineEvent* event_for_index(uint16_t index)
{
    list_node *l = list_get_head(&event_list.event_list_head);
    for (uint16_t i = 0; i < index;) {
        l = l->next;
        i++;
    }
    
    TimelineEvent *result = list_elem(l, TimelineEvent, node);
    
    return result;
}

static uint16_t _get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 1;
}

static uint16_t _get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return event_list.event_count;
}

static int16_t _get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context)
{
    return DISPLAY_ROWS;
}

static int16_t _get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return 0;
}

static void _draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context)
{
    TimelineEvent *event = event_for_index(cell_index->row);
    
    int16_t center = cell_layer->frame.size.h / 2;
    
    int time_len = strlen(event->time);
    char am_pm[3] = { event->time[time_len - 2], event->time[time_len - 1], '\0' };
    
    char time[time_len];
    memcpy(time, event->time, sizeof(time));
    time[time_len - 1] = '\0';
    time[time_len - 2] = '\0';
    
    GFont time_font = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
    GRect time_rect = GRect(0, center - 34, layer_get_frame(cell_layer).size.w - 65, 20);
    graphics_draw_text(ctx, time, time_font, time_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    
    GFont am_pm_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GRect am_pm_rect = GRect(0, center - 32, layer_get_frame(cell_layer).size.w - 45, 18);
    graphics_draw_text(ctx, am_pm, am_pm_font, am_pm_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    
    GFont name_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GRect name_rect = GRect(0, center - 15, layer_get_frame(cell_layer).size.w - 45, 18);
    graphics_draw_text(ctx, event->name, name_font, name_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    
    GFont desc_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    GRect desc_rect = GRect(0, center + 9, layer_get_frame(cell_layer).size.w - 45, 14);
    graphics_draw_text(ctx, event->description, desc_font, desc_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    
    // Sidebar for icon
    graphics_context_set_fill_color(ctx, event->color);
    graphics_fill_rect(ctx, GRect(layer_get_frame(cell_layer).size.w - 40, 0, 40, layer_get_frame(cell_layer).size.h), 0, GCornerNone);
    
    // Draw a triangle path as a cutout in the icon sidebar
    n_GPathInfo TRIANGLE_PATH_INFO = {
        .num_points = 3,
        .points = (GPoint []) {{layer_get_frame(cell_layer).size.w - 35, center}, {layer_get_frame(cell_layer).size.w - 41, center - 12 }, {layer_get_frame(cell_layer).size.w - 41, center + 12 }}
    };
    n_GPath *triangle_path = n_gpath_create(&TRIANGLE_PATH_INFO);
    graphics_context_set_fill_color(ctx, GColorWhite);
    n_gpath_fill(ctx, triangle_path);
    
    // Draw the icon
    GSize icon_size = event->icon->raw_bitmap_size;
    graphics_draw_bitmap_in_rect(ctx, event->icon, GRect(layer_get_frame(cell_layer).size.w - 30, center - (icon_size.h / 2), icon_size.w, icon_size.h));
    
}

static void _draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    //menu_cell_basic_header_draw(ctx, cell_layer, "Timeline");
}

static void _exit_callback(ClickRecognizerRef recognizer, void* context)
{
    window_stack_pop(true);
}

static void _event_notification_click_provider_config(void *context)
{
    window_single_click_subscribe(BUTTON_ID_BACK, _exit_callback);
}

static void _select_item(ClickRecognizerRef recognizer, void* context)
{
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    TimelineEvent *event = event_for_index(selected_index.row);
    
    // Show a Notification for the timeline event
    Window *notification_window = window_create();
    
    NotificationLayer *event_notification_layer = notification_layer_create(menu_layer_get_layer(s_menu_layer)->bounds);
    Notification *notification = notification_create(event->app, event->name, event->description, event->icon, event->color);
    notification_layer_stack_push_notification(event_notification_layer, notification);
    layer_add_child(window_get_root_layer(notification_window), notification_layer_get_layer(event_notification_layer));
    
    window_stack_push(notification_window, true);
    window_set_click_config_provider(notification_window, _event_notification_click_provider_config);
}

static void _click_provider_config(void *context)
{
    window_single_click_subscribe(BUTTON_ID_BACK, _exit_callback);
    window_single_click_subscribe(BUTTON_ID_SELECT, _select_item);
}

void timeline_init(void)
{
    timeline_window = window_create();
    Layer *window_layer = window_get_root_layer(timeline_window);
    GRect frame = layer_get_frame(window_layer);
    
    event_list = (TimelineEventList) { .event_count = 0 };
    list_init_head(&event_list.event_list_head);
    
    // Add some fake events
    GBitmap* icon = gbitmap_create_with_resource(RESOURCE_ID_SPEECH_BUBBLE);
    TimelineEvent *event = TIMELINE_EVENT("Sunset", "6:14PM", "-/60º", "The Weather Channel", icon, GColorVividCerulean);
    timeline_add_event(event);
    
    GBitmap* alarm_icon = gbitmap_create_with_resource(RESOURCE_ID_ALARM_BELL_RINGING);
    TimelineEvent *event2 = TIMELINE_EVENT("Alarm", "7:30AM", "Take out trash", "Alarms", alarm_icon, GColorFromRGB(24, 169, 88));
    timeline_add_event(event2);
    
    s_menu_layer = menu_layer_create(frame);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = _get_num_sections,
        .get_num_rows = _get_num_rows,
        .get_cell_height = _get_cell_height,
        .draw_row = _draw_row
    });
    menu_layer_set_click_config_provider(s_menu_layer, _click_provider_config);
    menu_layer_set_click_config_onto_window(s_menu_layer, timeline_window);
    menu_layer_set_highlight_colors(s_menu_layer, GColorWhite, GColorBlack);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    
    window_stack_push(timeline_window, true);
}

TimelineEvent* create_timeline_event(const char *time, const char *name, const char *description, const char *app, GBitmap *icon, GColor color)
{
    TimelineEvent *event = app_calloc(1, sizeof(TimelineEvent));
    event->time = time;
    event->name = name;
    event->description = description;
    event->app = app;
    event->icon = icon;
    event->color = color;
    
    return event;
}

void timeline_add_event(TimelineEvent *event)
{
    list_init_node(&event->node);
    list_insert_head(&event_list.event_list_head, &event->node);
    event_list.event_count++;
}

void timeline_deinit(void)
{
    layer_remove_from_parent(menu_layer_get_layer(s_menu_layer));
    menu_layer_destroy(s_menu_layer);
    window_destroy(timeline_window);
}

void timeline_main(void)
{
    timeline_init();
    app_event_loop();
    timeline_deinit();
}
