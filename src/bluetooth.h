#pragma once

#include "pebble.h"

// Sizes 
#define BT_W 9
#define BT_H 9

#define BLUETOOTH_REFRESH_INTERVAL (30 * 60 * 1000) // 30 minutes in miliseconds

void bluetooth_window_load(Window *window);
void bluetooth_window_unload(Window *window);

void bluetooth_init(Window *window);
void bluetooth_deinit(void);
