#include <pebble.h>

#define SETTINGS_KEY 1

typedef uint32_t numeral_t;

// corners are in the order:
//  | 0 1
//  | 2 3
//
// the line on the left is the middle line of the numeral
typedef struct Segment {
    uint8_t corner1;
    uint8_t corner2;
} Segment;

#define SEGMENT_TOP         ((Segment) { 1, 0 })
#define SEGMENT_RIGHT       ((Segment) { 1, 3 })
#define SEGMENT_BOTTOM      ((Segment) { 3, 2 })
#define SEGMENT_LEFT        ((Segment) { 2, 0 })
#define SEGMENT_DOWNWARDS   ((Segment) { 3, 0 })
#define SEGMENT_UPWARDS     ((Segment) { 2, 1 })

static Window* s_window;
static Layer* s_time_layer;
static Layer* s_date_layer;

typedef enum DateFormat {
    DateFormat_MMDD = 0,
    DateFormat_DDMM = 1,
} DateFormat;

typedef struct ClaySettings {
    bool AntiAliasing;
    bool ShowDate;
    DateFormat DateFormat;
    int32_t Padding;
    int32_t TimeStrokeWidth;
    int32_t DateStrokeWidth;
    int32_t DateScalePercent;
    GColor StrokeColor;
    GColor BackgroundColor;
} ClaySettings;

static ClaySettings settings;

static void default_settings(void) {
    settings.AntiAliasing = true;
    settings.ShowDate = true;
    settings.Padding = 25;
    settings.TimeStrokeWidth = 7;
    settings.DateStrokeWidth = 7;
    settings.DateScalePercent = 28;
    settings.StrokeColor = GColorBlack;
    settings.BackgroundColor = GColorWhite;
}

static void load_settings(void) {
    default_settings();
    persist_read_data(SETTINGS_KEY, &settings, sizeof settings);
}

static void log_settings(void) {
#define LOG_FIELD(name, fmt) \
    APP_LOG(APP_LOG_LEVEL_INFO, "settings." #name ": " fmt, settings.name)

    LOG_FIELD(AntiAliasing, "%d");
    LOG_FIELD(ShowDate, "%d");
    LOG_FIELD(Padding, "%d");
    LOG_FIELD(TimeStrokeWidth, "%d");
    LOG_FIELD(DateStrokeWidth, "%d");
    LOG_FIELD(DateScalePercent, "%d");
    LOG_FIELD(StrokeColor, "%02x");
    LOG_FIELD(BackgroundColor, "%02x");
#undef LOG_FIELD
}

static void save_settings(void) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof settings);
}

static void window_unload(Window* window);
static void window_load(Window* window);

static void apply_settings(void) {
    window_unload(s_window);
    window_load(s_window);
}

static void update_settings(DictionaryIterator* iterator) {
    bool changed = false;

#define FIELD(name, expr) \
    do { \
        Tuple* field = dict_find(iterator, MESSAGE_KEY_##name); \
        if (field) { \
            settings.name = (expr); \
            changed = true; \
        } \
    } while (0)

    FIELD(AntiAliasing, field->value->int32 == 1);
    FIELD(ShowDate, field->value->int32 == 1);
    FIELD(DateFormat, field->value->int32);
    FIELD(Padding, field->value->int32);
    FIELD(TimeStrokeWidth, field->value->int32);
    FIELD(DateStrokeWidth, field->value->int32);
    FIELD(DateScalePercent, field->value->int32);
    FIELD(StrokeColor, GColorFromHEX(field->value->int32));
    FIELD(BackgroundColor, GColorFromHEX(field->value->int32));
#undef FIELD

    if (changed) {
        save_settings();
        apply_settings();
        log_settings();
    }
}

static void inbox_received_callback(DictionaryIterator* iterator, void* context) {
    update_settings(iterator);
}

static void inbox_dropped_callback(AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "message dropped");
}

static GRect pad_rect(GRect rect, int16_t padding) {
    rect.size.w -= padding * 2;
    rect.size.h -= padding * 2;
    rect.origin.x += padding;
    rect.origin.y += padding;
    return rect;
}

// using the top right (section 0) as the reference point
static Segment normalize_segment_by_section(Segment segment, uint8_t section) {
    static uint8_t const corner_to_corner_in_section[4][4] = {
        {0, 1, 2, 3},
        {1, 0, 3, 2},
        {2, 3, 0, 1},
        {3, 2, 1, 0},
    };

    return (Segment) {
        .corner1 = corner_to_corner_in_section[section][segment.corner1],
        .corner2 = corner_to_corner_in_section[section][segment.corner2],
    };
}

static void draw_segment(GContext* ctx, GRect bounds, Segment segment) {
    // note:
    //  this can be optimized by calculating the points on-demand.
    //  maybe the compiler already does that, but I'm not entirely sure.
    GPoint const corners[4] = {
        bounds.origin,
        GPoint(bounds.origin.x + bounds.size.w, bounds.origin.y),
        GPoint(bounds.origin.x, bounds.origin.y + bounds.size.h),
        GPoint(bounds.origin.x + bounds.size.w, bounds.origin.y + bounds.size.h),
    };

    GPoint const p1 = corners[segment.corner1];
    GPoint const p2 = corners[segment.corner2];

    graphics_draw_line(ctx, p1, p2);
}

// sections are in a different order.
// the order of the sections is the order of the "digits":
// top right is section 0, top left is section 1, etc.
static void draw_section(GContext* ctx, GRect bounds, uint8_t value, uint8_t section) {
    // helper macro to reduce code duplication
#define SEGMENT(s) draw_segment(ctx, bounds, normalize_segment_by_section((s), section))
    switch (value) {
    break; case 0:

    break; case 1:
        SEGMENT(SEGMENT_TOP);

    break; case 2:
        SEGMENT(SEGMENT_BOTTOM);

    break; case 3:
        SEGMENT(SEGMENT_DOWNWARDS);

    break; case 4:
        SEGMENT(SEGMENT_UPWARDS);

    break; case 5:
        SEGMENT(SEGMENT_TOP);
        SEGMENT(SEGMENT_UPWARDS);

    break; case 6:
        SEGMENT(SEGMENT_RIGHT);

    break; case 7:
        SEGMENT(SEGMENT_TOP);
        SEGMENT(SEGMENT_RIGHT);

    break; case 8:
        SEGMENT(SEGMENT_BOTTOM);
        SEGMENT(SEGMENT_RIGHT);

    break; case 9:
        SEGMENT(SEGMENT_TOP);
        SEGMENT(SEGMENT_BOTTOM);
        SEGMENT(SEGMENT_RIGHT);

    break; default:;
    }
#undef SEGMENT
}

static void draw_numeral(GContext* ctx, GRect bounds, numeral_t value) {
    int16_t const size = bounds.size.w / 2;

    // draw the middle line
    graphics_draw_line(
        ctx,
        GPoint(bounds.origin.x + size, bounds.origin.y),
        GPoint(bounds.origin.x + size, bounds.origin.y + bounds.size.h)
    );

    // TODO:
    //  handle gap <= 0 (either a square or a horizontal rectangle)
    //  maybe do it with a configurable `min_gap`?
    int16_t const gap = bounds.size.h - size * 2;
    int16_t const bottom_y = gap + size;

    GPoint const sections_origins[4] = {
        GPoint(bounds.origin.x + size,  bounds.origin.y),
        GPoint(bounds.origin.x,         bounds.origin.y),
        GPoint(bounds.origin.x + size,  bounds.origin.y + bottom_y),
        GPoint(bounds.origin.x,         bounds.origin.y + bottom_y),
    };

    for (uint8_t section = 0; section < 4; section++, value /= 10) {
        GRect const section_bounds = {
            .origin = sections_origins[section],
            .size = {size, size}
        };

        draw_section(ctx, section_bounds, value % 10, section);
    }
}

static void draw_time(Layer* layer, GContext* ctx) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "draw time");
    GRect const bounds = layer_get_bounds(layer);

    time_t const unix_now = time(NULL);
    struct tm* const local_now = localtime(&unix_now);

    graphics_context_set_antialiased(ctx, settings.AntiAliasing);
    graphics_context_set_stroke_color(ctx, settings.StrokeColor);
    graphics_context_set_stroke_width(ctx, settings.TimeStrokeWidth);

    numeral_t const numeral = local_now->tm_min + local_now->tm_hour * 100;
    draw_numeral(ctx, bounds, numeral);
}

static void draw_date(Layer* layer, GContext* ctx) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "draw date");
    GRect const bounds = layer_get_bounds(layer);

    time_t const unix_now = time(NULL);
    struct tm* const local_now = localtime(&unix_now);

    graphics_context_set_antialiased(ctx, settings.AntiAliasing);
    graphics_context_set_stroke_color(ctx, settings.StrokeColor);
    graphics_context_set_stroke_width(ctx, settings.DateStrokeWidth);

    numeral_t numeral;
    if (settings.DateFormat == DateFormat_MMDD) {
        numeral = local_now->tm_mday + (local_now->tm_mon + 1) * 100;
    } else {
        numeral = (local_now->tm_mon + 1) + local_now->tm_mday * 100;
    }

    draw_numeral(ctx, bounds, numeral);
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed) {
    layer_mark_dirty(s_time_layer);
    layer_mark_dirty(s_date_layer);
}

static GRect calculate_date_bounds(GRect time_bounds) {
    int16_t const section_size = time_bounds.size.w / 2;

    int16_t const w = time_bounds.size.w * settings.DateScalePercent / 100;
    int16_t const h = time_bounds.size.h * settings.DateScalePercent / 100;
    int16_t const x = time_bounds.origin.x + (section_size - w) / 2;
    int16_t const y = time_bounds.origin.y + (section_size - h) / 2 + time_bounds.size.h - section_size;

    return GRect(x, y, w, h);
}

static void window_load(Window* window) {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    GRect time_bounds = pad_rect(bounds, settings.Padding);
    s_time_layer = layer_create(time_bounds);
    layer_set_update_proc(s_time_layer, draw_time);
    layer_set_clips(s_time_layer, false);
    layer_add_child(window_layer, s_time_layer);

    GRect date_bounds = calculate_date_bounds(time_bounds);
    s_date_layer = layer_create(date_bounds);
    layer_set_update_proc(s_date_layer, draw_date);
    layer_set_clips(s_date_layer, false);
    layer_set_hidden(s_date_layer, !settings.ShowDate);
    layer_add_child(window_layer, s_date_layer);
}

static void window_unload(Window* window) {
    layer_destroy(s_time_layer);
    layer_destroy(s_date_layer);
}

static void init(void) {
    load_settings();
    s_window = window_create();
    window_set_background_color(s_window, settings.BackgroundColor);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);

    const int inbox_size = 128;
    const int outbox_size = 0;
    app_message_open(inbox_size, outbox_size);
}

static void deinit(void) {
    window_destroy(s_window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    deinit();
}
