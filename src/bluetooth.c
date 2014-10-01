#include "pebble.h"

#include "settings.h"
#include "bluetooth.h"

static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

extern Layer *hands_layer;

static void bluetooth_update_proc(bool connected) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth state: %d ", connected);

  if (icon_bitmap) gbitmap_destroy(icon_bitmap);

  if (connected) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  else icon_bitmap = NULL;

  bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
}

void bluetooth_window_load(Window *window) {
  if (!INC_BLUETOOTH) return;

/* Weather layers */
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // icon 
  icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) + 10, 55, BT_W, BT_H));
  //layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
  // Add it below watch hands
  layer_insert_below_sibling(bitmap_layer_get_layer(icon_layer), hands_layer);

  // Check current charging status
  bluetooth_update_proc(bluetooth_connection_service_peek());
  // And register automatic callback
  bluetooth_connection_service_subscribe(bluetooth_update_proc);
}

void bluetooth_window_unload(Window *window) {
  if (!INC_BLUETOOTH) return;

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  bitmap_layer_destroy(icon_layer);
}

void bluetooth_init(Window *window) {
  if (!INC_BLUETOOTH) return;
}

void bluetooth_deinit(void) {
  if (!INC_BLUETOOTH) return;
}

