#pragma once

#include "pebble.h"

#define DATE_DAY_W 50
#define DATE_DAY_H 20

void analogface_window_load(Window *window);
void analogface_window_unload(Window *window);

void analogface_init(Window *window);
void analogface_deinit(void);
