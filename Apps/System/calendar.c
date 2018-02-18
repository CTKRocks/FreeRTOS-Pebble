/* calendar.c
 * A simple calendar app
 *
 * Author: Carson Katri <me@carsonkatri.com>
 */

#include "librebble.h"
#include "graphics_wrapper.h"
#include "menu_layer.h"

static MenuLayer* s_menu_layer;
static Window* calendar_window;
static uint16_t year = 2018;
static const char* months[] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September","October", "November", "December"
};

static uint16_t days_in_month(uint16_t month, uint16_t year)
{
    if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;
    else if (month == 2)
    {
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (isLeapYear)
            return 29;
        else
            return 28;
    }
    else
        return 31;
}

static uint16_t _get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 12;
}

static uint16_t _get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return days_in_month(section + 1, year);
}

static int16_t _get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context)
{
    return 20;
}

static int16_t _get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return days_in_month(section, year) > 28 ? 48 : 24;
}

static void _draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context)
{
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    char text[3];
    snprintf(text, sizeof(text), "%d", cell_index->row + 1);
    GRect rect = { .origin = GPoint(0, 0), .size = layer_get_frame(cell_layer).size };
    graphics_draw_text(ctx, text, font, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void _draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    menu_cell_basic_header_draw(ctx, cell_layer, months[section]);
}

static void _select_item(ClickRecognizerRef recognizer, void* context)
{
    // Launch the timeline for this day
    appmanager_app_start("Timeline");
}

static void _exit_callback(ClickRecognizerRef recognizer, void* context)
{
    
}

static void _click_provider_config(void *context)
{
    window_single_click_subscribe(BUTTON_ID_BACK, _exit_callback);
    window_single_click_subscribe(BUTTON_ID_SELECT, _select_item);
}

void calendar_init(void)
{
    calendar_window = window_create();
    Layer *window_layer = window_get_root_layer(calendar_window);
    GRect frame = layer_get_frame(window_layer);
    
    s_menu_layer = menu_layer_create(frame);
    menu_layer_set_column_count(s_menu_layer, 7);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = _get_num_sections,
        .get_num_rows = _get_num_rows,
        .get_cell_height = _get_cell_height,
        .get_header_height = _get_header_height,
        .draw_row = _draw_row,
        .draw_header = _draw_header
    });
    menu_layer_set_click_config_provider(s_menu_layer, _click_provider_config);
    menu_layer_set_click_config_onto_window(s_menu_layer, calendar_window);
    menu_layer_set_highlight_colors(s_menu_layer, GColorVividCerulean, GColorWhite);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    
    window_stack_push(calendar_window, true);
}

void calendar_deinit(void)
{
    layer_remove_from_parent(menu_layer_get_layer(s_menu_layer));
    menu_layer_destroy(s_menu_layer);
    window_destroy(calendar_window);
}

void calendar_main(void)
{
    calendar_init();
    app_event_loop();
    calendar_deinit();
}
