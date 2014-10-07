#include "pebble.h"

#define DEBUG_ON
#include "debug.h"

#include "settings.h"
#include "weather.h"

#define WEATHER_TEMP_LENGTH 6

static TextLayer *temperature_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static Layer *hidden_bg_layer;
static TextLayer *hidden_temperature_layer;
static TextLayer *hidden_city_layer;
static TextLayer *hidden_time_layer;
static TextLayer *hidden_desc_layer;
static TextLayer *hidden_sunrise_txt_layer;
static TextLayer *hidden_sunset_txt_layer;
static TextLayer *hidden_remaining_txt_layer;
static BitmapLayer *hidden_icon_layer;
static BitmapLayer *hidden_sunrise_layer;
static BitmapLayer *hidden_sunset_layer;
static GBitmap *hidden_icon_bitmap = NULL;
static GBitmap *hidden_sunrise_bitmap = NULL;
static GBitmap *hidden_sunset_bitmap = NULL;

extern Layer *hands_layer;

static AppSync sync;
static uint8_t sync_buffer[1024];

static AppTimer* hide_window_timer = NULL;

#define OPENWEATHER_NUM_ICONS 21
#define OPENWEATHER_ICON_SIZE 4
#define OPENWEATHER_TEMP_SIZE 4

typedef struct WeatherIcon {
  char icon[OPENWEATHER_ICON_SIZE];
  uint32_t resource_id;
} WeatherIcon;

const WeatherIcon weather_icons_16[OPENWEATHER_NUM_ICONS] = {
  {"xxx", RESOURCE_ID_IMAGE_16_EMPTY}, // default empty icon  
  {"01d", RESOURCE_ID_IMAGE_16_01d}, // clear sky
  {"01n", RESOURCE_ID_IMAGE_16_01n}, 
  {"02d", RESOURCE_ID_IMAGE_16_02d}, // few clouds
  {"02n", RESOURCE_ID_IMAGE_16_02n}, 
  {"03d", RESOURCE_ID_IMAGE_16_03d}, // scattered clouds
  {"03n", RESOURCE_ID_IMAGE_16_03n}, 
  {"04d", RESOURCE_ID_IMAGE_16_04d}, // broken clouds
  {"04n", RESOURCE_ID_IMAGE_16_04n}, 
  {"09d", RESOURCE_ID_IMAGE_16_09d}, // shower rain
  {"09n", RESOURCE_ID_IMAGE_16_09n}, 
  {"10d", RESOURCE_ID_IMAGE_16_10d}, // rain
  {"10n", RESOURCE_ID_IMAGE_16_10n}, 
  {"11d", RESOURCE_ID_IMAGE_16_11d}, // thunderstorm
  {"11n", RESOURCE_ID_IMAGE_16_11n}, 
  {"13d", RESOURCE_ID_IMAGE_16_13d}, // snow
  {"13n", RESOURCE_ID_IMAGE_16_13n}, 
  {"50d", RESOURCE_ID_IMAGE_16_50d}, // mist
  {"50n", RESOURCE_ID_IMAGE_16_50n} ,
  {"srs", RESOURCE_ID_IMAGE_16_SUNRISE}, // sunrise
  {"sst", RESOURCE_ID_IMAGE_16_SUNSET}   // sunset
};

const WeatherIcon weather_icons_32[OPENWEATHER_NUM_ICONS] = {
  {"xxx", RESOURCE_ID_IMAGE_32_UPDATE},
  {"01d", RESOURCE_ID_IMAGE_32_01d}, // clear sky
  {"01n", RESOURCE_ID_IMAGE_32_01n}, 
  {"02d", RESOURCE_ID_IMAGE_32_02d}, // few clouds
  {"02n", RESOURCE_ID_IMAGE_32_02n}, 
  {"03d", RESOURCE_ID_IMAGE_32_03d}, // scattered clouds
  {"03n", RESOURCE_ID_IMAGE_32_03n}, 
  {"04d", RESOURCE_ID_IMAGE_32_04d}, // broken clouds
  {"04n", RESOURCE_ID_IMAGE_32_04n}, 
  {"09d", RESOURCE_ID_IMAGE_32_09d}, // shower rain
  {"09n", RESOURCE_ID_IMAGE_32_09n}, 
  {"10d", RESOURCE_ID_IMAGE_32_10d}, // rain
  {"10n", RESOURCE_ID_IMAGE_32_10n}, 
  {"11d", RESOURCE_ID_IMAGE_32_11d}, // thunderstorm
  {"11n", RESOURCE_ID_IMAGE_32_11n}, 
  {"13d", RESOURCE_ID_IMAGE_32_13d}, // snow
  {"13n", RESOURCE_ID_IMAGE_32_13n}, 
  {"50d", RESOURCE_ID_IMAGE_32_50d}, // mist
  {"50n", RESOURCE_ID_IMAGE_32_50n},
  {"srs", RESOURCE_ID_IMAGE_16_SUNRISE}, // sunrise
  {"sst", RESOURCE_ID_IMAGE_16_SUNSET}   // sunset
};

uint32_t find_icon_resource(const WeatherIcon* icons, const char* icon) {
  for (unsigned int i = 0; i < OPENWEATHER_NUM_ICONS; i++)
  {
    if (strcmp(icons[i].icon, icon) == 0)
    {
      return icons[i].resource_id;
    }
  }
  // If not found, return default icon
  return icons[0].resource_id;
}

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_CSTRING
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_TEMPERATURE_HID_KEY = 0x2,
  WEATHER_CITY_KEY = 0x3,         // TUPLE_CSTRING
  WEATHER_DESC_KEY = 0x4,         // TUPLE_CSTRING
  WEATHER_SUNRISE_KEY = 0x5,
  WEATHER_SUNSET_KEY  = 0x6,
  WEATHER_REMAINING_KEY = 0x7,
  WEATHER_DT_KEY = 0x8,
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Callback %d %s", (int)key, new_tuple->value->cstring);
  switch (key) {
    case WEATHER_ICON_KEY: {
        if (icon_bitmap) {
          gbitmap_destroy(icon_bitmap);
        }
        uint32_t icon_resource = find_icon_resource(weather_icons_16, new_tuple->value->cstring);
        icon_bitmap = gbitmap_create_with_resource(icon_resource);
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);


        if (hidden_icon_bitmap) {
          gbitmap_destroy(hidden_icon_bitmap);
        }
        icon_resource = find_icon_resource(weather_icons_32, new_tuple->value->cstring);
        hidden_icon_bitmap = gbitmap_create_with_resource(icon_resource);
        bitmap_layer_set_bitmap(hidden_icon_layer, hidden_icon_bitmap);
      }
      break;

    case WEATHER_TEMPERATURE_KEY: {
        /*static char time_str[] = "Fri Oct  3 20:49:12 2014"; // Needs to be static because it's used by the system later.
        time_t current_time = time(NULL);
        strftime(time_str, 25, "%c", localtime(&current_time));*/
        //text_layer_set_text(hidden_time_layer, time_str);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Update time %s", time_str);

        text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      }
      break;

    case WEATHER_TEMPERATURE_HID_KEY: {
        text_layer_set_text(hidden_temperature_layer, new_tuple->value->cstring);
      }
      break;

    case WEATHER_CITY_KEY: {
      //text_layer_set_text(city_layer, new_tuple->value->cstring);
        text_layer_set_text(hidden_city_layer, new_tuple->value->cstring);
      }
      break;

    case WEATHER_DESC_KEY: {
        text_layer_set_text(hidden_desc_layer, new_tuple->value->cstring);      
      }
      break;

    case WEATHER_SUNRISE_KEY: {
        text_layer_set_text(hidden_sunrise_txt_layer, new_tuple->value->cstring);      
      }
      break;

    case WEATHER_SUNSET_KEY: {
        text_layer_set_text(hidden_sunset_txt_layer, new_tuple->value->cstring);      
      }
      break;

    case WEATHER_REMAINING_KEY: {
        text_layer_set_text(hidden_remaining_txt_layer, new_tuple->value->cstring);      
      }
      break;

    case WEATHER_DT_KEY: {
        text_layer_set_text(hidden_time_layer, new_tuple->value->cstring);
      }

  }
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

/*weather_details_load(void) {
  layer_set_hidden(hidden_bg_layer, false);

}*/

static void hidden_bg_layer_draw(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void weather_window_load(Window *window) {
  if (!INC_WEATHER) return;

 /* Weather layers */
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // icon 
  icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - ICON_W - 5, (bounds.size.h / 2) + 23 , ICON_W, ICON_H));
  //bitmap_layer_set_compositing_mode(icon_layer, GCompOpAssignInverted);
  // layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
  // Add it below watch hands
  layer_insert_below_sibling(bitmap_layer_get_layer(icon_layer), hands_layer);
 
  // temperature
  temperature_layer = text_layer_create(GRect((bounds.size.w / 2), (bounds.size.h / 2) + 27, TEMP_W, TEMP_H));
  text_layer_set_text_color(temperature_layer, GColorWhite);
  text_layer_set_background_color(temperature_layer, GColorClear);
  text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentLeft);
  // layer_add_child(window_layer, text_layer_get_layer(temperature_layer));
  // Add it below watch hands
  layer_insert_below_sibling(text_layer_get_layer(temperature_layer), hands_layer);

  /*
   *Hidden part
   */

  hidden_bg_layer = layer_create(bounds);
  layer_set_update_proc(hidden_bg_layer, hidden_bg_layer_draw);
  layer_set_hidden(hidden_bg_layer, true);
  layer_add_child(window_layer, hidden_bg_layer);

  // Hidden time of update
  hidden_time_layer = text_layer_create(GRect(bounds.size.w - TIME_W, 0, TIME_W, TEMP_H));
  text_layer_set_text_color(hidden_time_layer, GColorWhite);
  text_layer_set_background_color(hidden_time_layer, GColorClear);
  text_layer_set_font(hidden_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(hidden_time_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_time_layer));

  // Hidden city
  hidden_city_layer = text_layer_create(GRect(0, 0, bounds.size.w, TEMP_H));
  text_layer_set_text_color(hidden_city_layer, GColorWhite);
  text_layer_set_background_color(hidden_city_layer, GColorClear);
  text_layer_set_font(hidden_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(hidden_city_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_city_layer));

  // Hiden icon
  hidden_icon_layer = bitmap_layer_create(GRect(0, 20 + 4, ICON_W, ICON_H));
  layer_add_child(hidden_bg_layer, bitmap_layer_get_layer(hidden_icon_layer));

  hidden_sunrise_layer = bitmap_layer_create(GRect(0, bounds.size.h - ICON_H, ICON_W, ICON_H));
  hidden_sunrise_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_16_SUNRISE);  
  bitmap_layer_set_bitmap(hidden_sunrise_layer, hidden_sunrise_bitmap);
  layer_add_child(hidden_bg_layer, bitmap_layer_get_layer(hidden_sunrise_layer));  

  hidden_sunset_layer = bitmap_layer_create(GRect(bounds.size.w / 2, bounds.size.h - ICON_H, ICON_W, ICON_H));
  hidden_sunset_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_16_SUNSET);  
  bitmap_layer_set_bitmap(hidden_sunset_layer, hidden_sunset_bitmap);
  layer_add_child(hidden_bg_layer, bitmap_layer_get_layer(hidden_sunset_layer));

  // Hidden temperature
  hidden_temperature_layer = text_layer_create(GRect(ICON_W + 2, 20, TEMP_W, TEMP_H));
  text_layer_set_text_color(hidden_temperature_layer, GColorWhite);
  text_layer_set_background_color(hidden_temperature_layer, GColorClear);
  text_layer_set_font(hidden_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(hidden_temperature_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_temperature_layer));

  // Hidden description
  hidden_desc_layer = text_layer_create(GRect(ICON_W + 2, 20 + TEMP_H, bounds.size.w - (ICON_W + 2), TEMP_H));
  text_layer_set_text_color(hidden_desc_layer, GColorWhite);
  text_layer_set_background_color(hidden_desc_layer, GColorClear);
  text_layer_set_font(hidden_desc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(hidden_desc_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_desc_layer));

  // Hidden remianing text
  hidden_remaining_txt_layer = text_layer_create(GRect(0, 20 + TEMP_H * 2, bounds.size.w, TEMP_H * 3));
  text_layer_set_text_color(hidden_remaining_txt_layer, GColorWhite);
  text_layer_set_background_color(hidden_remaining_txt_layer, GColorClear);
  text_layer_set_font(hidden_remaining_txt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(hidden_remaining_txt_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_remaining_txt_layer));

  // Hidden sunrise
  hidden_sunrise_txt_layer = text_layer_create(GRect(ICON_W, bounds.size.h - TEMP_H, (bounds.size.w / 2) - ICON_W, TEMP_H));
  text_layer_set_text_color(hidden_sunrise_txt_layer, GColorWhite);
  text_layer_set_background_color(hidden_sunrise_txt_layer, GColorClear);
  text_layer_set_font(hidden_sunrise_txt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(hidden_sunrise_txt_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_sunrise_txt_layer));

  // Hidden sunset
  hidden_sunset_txt_layer = text_layer_create(GRect((bounds.size.w / 2) + ICON_W, bounds.size.h - TEMP_H, (bounds.size.w / 2) - ICON_W, TEMP_H));
  text_layer_set_text_color(hidden_sunset_txt_layer, GColorWhite);
  text_layer_set_background_color(hidden_sunset_txt_layer, GColorClear);
  text_layer_set_font(hidden_sunset_txt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(hidden_sunset_txt_layer, GTextAlignmentLeft);
  layer_add_child(hidden_bg_layer, text_layer_get_layer(hidden_sunset_txt_layer));

  // init App sync
  Tuplet initial_values[] = {
    TupletCString(WEATHER_ICON_KEY, ""),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletCString(WEATHER_TEMPERATURE_HID_KEY, ""),
    TupletCString(WEATHER_CITY_KEY, ""),
    TupletCString(WEATHER_DESC_KEY, ""),
    TupletCString(WEATHER_SUNRISE_KEY, ""),
    TupletCString(WEATHER_SUNSET_KEY, ""),
    TupletCString(WEATHER_REMAINING_KEY, ""),
    TupletCString(WEATHER_DT_KEY, ""),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();
}

void weather_window_unload(Window *window) {
  if (!INC_WEATHER) return;

  app_sync_deinit(&sync);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }
  text_layer_destroy(temperature_layer);
  bitmap_layer_destroy(icon_layer);

  if (hidden_icon_bitmap) {
    gbitmap_destroy(hidden_icon_bitmap);
  }
  if (hidden_sunrise_bitmap) {
    gbitmap_destroy(hidden_sunrise_bitmap);
  }
  if (hidden_sunset_bitmap) {
    gbitmap_destroy(hidden_sunset_bitmap);
  }
  bitmap_layer_destroy(hidden_icon_layer);
  bitmap_layer_destroy(hidden_sunrise_layer);
  bitmap_layer_destroy(hidden_sunset_layer);
  text_layer_destroy(hidden_temperature_layer);
  text_layer_destroy(hidden_time_layer);
  text_layer_destroy(hidden_city_layer);
  text_layer_destroy(hidden_desc_layer);
  text_layer_destroy(hidden_remaining_txt_layer);
  text_layer_destroy(hidden_sunrise_txt_layer);
  text_layer_destroy(hidden_sunset_txt_layer);

}

void hide_window_handler(void* data) {
  if (hidden_bg_layer) layer_set_hidden(hidden_bg_layer, true);
}

void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
  // Direction is 1 or -1
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Accelerometer %s %ld", (axis==ACCEL_AXIS_X)?"X":(axis==ACCEL_AXIS_Y)?"Y":"Z", direction);

  if (axis == ACCEL_AXIS_X) {
    if (layer_get_hidden(hidden_bg_layer)) {
      // If the hidden screen is not displayed yet
      layer_set_hidden(hidden_bg_layer, false);
      hide_window_timer = app_timer_register(10 * 1000 /*10s*/, hide_window_handler, NULL);
    } else {
      /*if (hide_window_timer) app_timer_cancel(hide_window_timer);
      hide_window_timer = NULL;
      layer_set_hidden(hidden_bg_layer, true);*/
      
      // If we have the hidden screen already displayed
      if (hide_window_timer) app_timer_reschedule(hide_window_timer, 20 * 1000); // Set the timeout to 20 sec to wait for the update
      else hide_window_timer = app_timer_register(20 * 1000 /*10s*/, hide_window_handler, NULL);
      send_cmd(); // get updated data
    }
  }
}


void weather_init(Window *window) {
  if (!INC_WEATHER) return;

  accel_tap_service_subscribe(accel_tap_handler);

  // init app messages
  const int inbound_size = app_message_inbox_size_maximum();
  const int outbound_size = app_message_inbox_size_maximum();
  app_message_open(inbound_size, outbound_size);
}

void weather_deinit(void) {
  if (!INC_WEATHER) return;
  if (hide_window_timer) app_timer_cancel(hide_window_timer);
  hide_window_timer = NULL;
}
