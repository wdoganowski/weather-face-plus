#include "pebble.h"

#include <string.h>
#include <stdlib.h>

#include "settings.h"
#include "simple_analog.h"
#include "analog_face.h"
#include "weather.h"

#define DEBUG_OFF
#include "debug.h"

#define HANDS_UPDATE_STEP_MS 50

#ifdef SIMPLE_BACKGROUND
Layer *simple_bg_layer;
#else
BitmapLayer *simple_bg_layer;
#endif

Layer *date_layer;
TextLayer *day_label;
char day_buffer[11];

static GPath *minute_arrow;
static GPath *hour_arrow;
static GPath *tick_paths[NUM_CLOCK_TICKS];
Layer *hands_layer; 

static Window *the_window;

#ifdef SIMPLE_BACKGROUND
static void bg_update_proc(Layer *layer, GContext *ctx) {

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, tick_paths[i]);
  }
}
#endif

static AppTimer *timer = NULL;
GPoint move_xy/* = (GPoint) { .x = 0, .y = 0 }*/;

static void hands_move_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

  accel_service_peek(&accel);

  LOG_INFO("x %d y %d z %d", accel.x, accel.y, accel.z);
  move_xy.x = accel.x / -100;
  move_xy.y = accel.y / -100;

  layer_mark_dirty(hands_layer);

  timer = app_timer_register(HANDS_UPDATE_STEP_MS, hands_move_callback, NULL);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  const int16_t secondHandLength = bounds.size.w / 2;

  center.x += move_xy.x;
  center.y += move_xy.y;

  GPoint secondHand;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
  secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;

  // second hand
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, secondHand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, minute_arrow);
  gpath_draw_outline(ctx, minute_arrow);
  gpath_move_to(minute_arrow, center);

  gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, hour_arrow);
  gpath_draw_outline(ctx, hour_arrow);
  gpath_move_to(hour_arrow, center);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(center.x - 1, center.y - 1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(day_buffer, sizeof(day_buffer), "%a", t);
  day_buffer[3] = ' ';
  strncpy(&day_buffer[3], "  ", 2);
  strftime(&day_buffer[5], sizeof(day_buffer) - 5, "%d", t);

  text_layer_set_text(day_label, day_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(the_window));
}

/*
 * Exported
 */

void analogface_move_xy_setup(int enabled) {
  if (enabled) {
      timer = app_timer_register(HANDS_UPDATE_STEP_MS, hands_move_callback, NULL);
  } else {
      if (timer) app_timer_cancel(timer);
      timer = NULL;
      move_xy.x = 0;
      move_xy.y = 0;
  }
}

void analogface_window_load(Window *window) {
  if (!INC_ANALOGFACE) return;

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // init layers
#ifdef SIMPLE_BACKGROUND
  simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, simple_bg_layer);
#else
  simple_bg_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(simple_bg_layer, gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND));
  layer_add_child(window_layer, bitmap_layer_get_layer(simple_bg_layer));
#endif

  // init date layer -> a plain parent layer to create a date update proc
  date_layer = layer_create(bounds);
  layer_set_update_proc(date_layer, date_update_proc);
  layer_add_child(window_layer, date_layer);

  // init day
  day_label = text_layer_create(GRect((bounds.size.w / 2) - (DATE_DAY_W / 2), (bounds.size.h / 2) + 13, DATE_DAY_W, DATE_DAY_H));
  text_layer_set_text(day_label, day_buffer);
  text_layer_set_background_color(day_label, GColorBlack);
  text_layer_set_text_color(day_label, GColorWhite);
  text_layer_set_text_alignment(day_label, GTextAlignmentCenter);
  GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  text_layer_set_font(day_label, norm18);

  layer_add_child(date_layer, text_layer_get_layer(day_label));

  // init hands
  hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, hands_update_proc);
  layer_add_child(window_layer, hands_layer);

}

void analogface_window_unload(Window *window) {
  if (!INC_ANALOGFACE) return;

#ifdef SIMPLE_BACKGROUND
  layer_destroy(simple_bg_layer);
#else
  bitmap_layer_destroy(simple_bg_layer);
#endif
  layer_destroy(date_layer);
  text_layer_destroy(day_label);
  layer_destroy(hands_layer);
}

void analogface_init(Window *window) {
  if (!INC_ANALOGFACE) return;

  day_buffer[0] = '\0';
  the_window = window;

  // init hand paths
  minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  //Layer *window_layer = window_get_root_layer(window);
  // GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = {144 / 2, 168 / 2}; //grect_center_point(&bounds);
  gpath_move_to(minute_arrow, center);
  gpath_move_to(hour_arrow, center);

  // init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  // Seconds handle timer
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  // Hands move timer
  move_xy.x = 0;
  move_xy.y = 0;
  // timer = app_timer_register(HANDS_UPDATE_STEP_MS, hands_move_callback, NULL);
}

void analogface_deinit(void) {
  if (!INC_ANALOGFACE) return;

  if ( minute_arrow ) gpath_destroy( minute_arrow );
  if ( hour_arrow ) gpath_destroy( hour_arrow )
;}
