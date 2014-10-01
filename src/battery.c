#include "pebble.h"

#include "settings.h"
#include "battery.h"

static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

extern Layer *hands_layer;

static void battery_update_proc(BatteryChargeState charge) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Battery state: %d percent, chargin is %d, cable is %d", charge.charge_percent, charge.is_charging, charge.is_plugged);

  if (icon_bitmap) gbitmap_destroy(icon_bitmap);

  if (charge.is_plugged) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_CHARGING);
  else if (charge.charge_percent <= 10) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_2);
  else if (charge.charge_percent <= 20) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_3);
  else if (charge.charge_percent <= 40) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_4);
  else if (charge.charge_percent <= 60) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_5);
  else if (charge.charge_percent <= 80) icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_6);
  else icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_FULL);

  bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
}

void battery_window_load(Window *window) {
  if (!INC_BATTERY) return;

  // Weather layers
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // icon 
  icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - 17, 51, BAT_W, BAT_H));
  //layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
  // Add it below watch hands
  layer_insert_below_sibling(bitmap_layer_get_layer(icon_layer), hands_layer);

  // Check current charging status
  battery_update_proc(battery_state_service_peek());
  // And register automatic callback
  battery_state_service_subscribe(battery_update_proc);
}

void battery_window_unload(Window *window) {
  if (!INC_BATTERY) return;

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  bitmap_layer_destroy(icon_layer);
}

void battery_init(Window *window) {
  if (!INC_BATTERY) return;
}

void battery_deinit(void) {
  if (!INC_BATTERY) return;
}

