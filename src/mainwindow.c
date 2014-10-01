#include "pebble.h"

#include "mainwindow.h"

#include "settings.h"
#include "battery.h"
#include "analog_face.h"
#include "weather.h"
#include "bluetooth.h"

static Window *window;

static void window_load(Window *window) {
  analogface_window_load(window);
  weather_window_load(window);
  battery_window_load(window);
  bluetooth_window_load(window);
}

static void window_unload(Window *window) {
  bluetooth_window_unload(window);
  battery_window_unload(window);
  weather_window_unload(window);
  analogface_window_unload(window);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(click_recognizer_get_button_id(recognizer)) {
    case BUTTON_ID_UP: {
      break;
    }
    case BUTTON_ID_SELECT: {
      break;
    }
    case BUTTON_ID_DOWN: {
      break;
    }
    case NUM_BUTTONS: {
      break;
    }
    case BUTTON_ID_BACK: {
      break;
    }
  }
}

static void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

void main_window_init(void) {
  // Main window
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  // Init main window
  analogface_init(window);
  weather_init(window);
  battery_init(window);
  bluetooth_init(window);

  // Set key handler
  // window_set_click_config_provider(window, config_provider);

  // Open main window
  window_stack_push(window, true /* animated */);
}

void main_window_deinit(void) {
  bluetooth_deinit();
  battery_deinit();
  weather_deinit();
  analogface_deinit();

  window_destroy(window);
}
