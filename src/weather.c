#include "pebble.h"

#define DEBUG_ON
#include "debug.h"

#include "settings.h"
#include "weather.h"

#define WEATHER_TEMP_LENGTH 6

static TextLayer *temperature_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static AppSync sync;
static uint8_t sync_buffer[64];

extern Layer *hands_layer;

#define OPENWEATHER_NUM_ICONS 19

typedef struct WeatherIcon {
  char icon[4];
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
  {"50n", RESOURCE_ID_IMAGE_16_50n} 
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
  {"50n", RESOURCE_ID_IMAGE_32_50n} 
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
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case WEATHER_ICON_KEY: {
        if (icon_bitmap) {
          gbitmap_destroy(icon_bitmap);
        }
        uint32_t icon_resource = find_icon_resource(weather_icons_16, new_tuple->value->cstring);
        icon_bitmap = gbitmap_create_with_resource(icon_resource);
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      }
      break;

    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      break;

    case WEATHER_CITY_KEY:
      //text_layer_set_text(city_layer, new_tuple->value->cstring);
      break;
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

  // init App sync
  Tuplet initial_values[] = {
    TupletCString(WEATHER_ICON_KEY, ""),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletCString(WEATHER_CITY_KEY, ""),
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
}

void weather_init(Window *window) {
  if (!INC_WEATHER) return;

  // init app messages
  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
}

void weather_deinit(void) {
  if (!INC_WEATHER) return;
}
