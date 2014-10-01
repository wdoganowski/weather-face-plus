#pragma once

#include "pebble.h"

// Sizes 
#define BAT_W 16
#define BAT_H 16

#define BATTERY_REFRESH_INTERVAL (30 * 60 * 1000) // 30 minutes in miliseconds

void battery_window_load(Window *window);
void battery_window_unload(Window *window);

void battery_init(Window *window);
void battery_deinit(void);
