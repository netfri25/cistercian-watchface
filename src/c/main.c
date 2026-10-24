#include <pebble.h>

#define BACKGROUND_COLOR GColorDarkGray

typedef uint32_t numeral_t;

// corners are in the order:
//  0 1
//  2 3
typedef struct Segment {
    uint8_t corner1;
    uint8_t corner2;
} Segment;

#define SEGMENT_TOP         ((Segment) { 0, 1 })
#define SEGMENT_RIGHT       ((Segment) { 1, 3 })
#define SEGMENT_BOTTOM      ((Segment) { 3, 2 })
#define SEGMENT_LEFT        ((Segment) { 2, 0 })
#define SEGMENT_DOWNWARDS   ((Segment) { 0, 3 })
#define SEGMENT_UPWARDS     ((Segment) { 2, 1 })

static Window* s_window;
static Layer* s_time_layer;

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
    time_t now_unix = time(NULL);
    struct tm* local_now = localtime(&now_unix);

    GRect bounds = layer_get_bounds(layer);
    int16_t const padding = 25; // TODO: make configurable

    bounds.origin.x += padding;
    bounds.origin.y += padding;
    bounds.size.w -= padding * 2;
    bounds.size.h -= padding * 2;

    // TODO: make configurable
    graphics_context_set_antialiased(ctx, true);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 6);

    numeral_t numeral = local_now->tm_min + local_now->tm_hour * 100;
    draw_numeral(ctx, bounds, numeral);
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed) {
    layer_mark_dirty(s_time_layer);
}

static void window_load(Window* window) {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_time_layer = layer_create(bounds);
    layer_set_update_proc(s_time_layer, draw_time);
    layer_add_child(window_layer, s_time_layer);
}

static void window_unload(Window* window) {
    layer_destroy(s_time_layer);
}

static void init(void) {
    s_window = window_create();
    window_set_background_color(s_window, BACKGROUND_COLOR);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
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
