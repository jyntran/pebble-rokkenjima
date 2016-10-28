#include <pebble.h>

static Window *s_window;
static Layer *s_canvas_layer;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void bluetooth_callback(bool connected) {
  if (!connected) {
    vibes_double_pulse();
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // White clockface
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint16_t radius = bounds.size.w/2 - 8;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, centre, radius);

  // Border 
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, centre, radius-4);
  graphics_draw_circle(ctx, centre, radius-6);
  graphics_draw_circle(ctx, centre, radius/3);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawn canvas");
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  update_time();

  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
