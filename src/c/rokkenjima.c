#include <pebble.h>
#include <pdc-transform/pdc-transform.h>
#include "rokkenjima.h"

static Window *s_window;
static Layer *s_canvas_layer, *s_hands_layer;
static GDrawCommandImage *s_minute_hand, *s_hour_hand;
static GFont s_label_font;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void bluetooth_callback(bool connected) {
  window_set_background_color(s_window, GColorBlack);

  if (!connected) {
    window_set_background_color(s_window, GColorRed);
    vibes_double_pulse();
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // White clockface
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint16_t radius = bounds.size.w/2 - 2;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, centre, radius);

  // Border 
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, centre, radius-4);
  graphics_draw_circle(ctx, centre, radius-6);
  graphics_draw_circle(ctx, centre, radius/3);

  // Numbers
  struct Label LABELS[] = {
    {12, "XII"},
    {1, "I"},       
    {2, "II"},     
    {3, "III"},       
    {4, "IV"},       
    {5, "V"},       
    {6, "VI"},       
    {7, "VII"},       
    {8, "VIII"},       
    {9, "IX"},       
    {10, "X"},       
    {11, "XI"}      
  };

  for (int i=0; i <12; i++) {
    int angle = (TRIG_MAX_ANGLE * (i % 12) * 6) / (12 * 6);
    int x = (centre.x - (LABELWIDTH/2)) + (sin_lookup(angle) * (radius-LABELHEIGHT) / TRIG_MAX_RATIO);
    int y = (centre.y - (LABELHEIGHT/2)) + (-cos_lookup(angle) * (radius-LABELHEIGHT) / TRIG_MAX_RATIO);
  
    if (i==0) {
      graphics_context_set_text_color(ctx, GColorRed);
    } else {
      graphics_context_set_text_color(ctx, GColorBlack);
    }

    graphics_draw_text(ctx, LABELS[i].text, s_label_font, GRect(x, y, LABELWIDTH, LABELHEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);

  pdc_transform_gdraw_command_image_draw_transformed(ctx, s_minute_hand, GPoint(centre.x-8,centre.y-60), 10, 360 * t->tm_min / 60);

  pdc_transform_gdraw_command_image_draw_transformed(ctx, s_hour_hand, GPoint(centre.x-8,centre.y-50), 10, (360 * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));

  // dot in the middle
  uint32_t radius = bounds.size.w/2 - 2; 
  graphics_fill_circle(ctx, centre, radius/12);
  graphics_draw_circle(ctx, centre, radius/12);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_label_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LABEL_12));
  
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  layer_mark_dirty(s_canvas_layer);

  // Hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_hands_layer);
  layer_destroy(s_canvas_layer);
  fonts_unload_custom_font(s_label_font);
  gdraw_command_image_destroy(s_minute_hand);
  gdraw_command_image_destroy(s_hour_hand);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  s_minute_hand = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_MINUTE_HAND);
  s_hour_hand = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_HOUR_HAND);
  
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
  app_event_loop();
  prv_deinit();
}
