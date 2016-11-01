#include <pebble.h>
#include <pdc-transform/pdc-transform.h>
#include "rokkenjima.h"

static Window *s_window;
static Layer *s_clock_layer, *s_hands_layer;
static GDrawCommandImage *s_bg, *s_minute_hand, *s_hour_hand;
static GFont s_label_font;

struct ClaySettings settings;

static void prv_default_settings() {
  settings.BackgroundColour = GColorBlack;
  settings.ClockColour = PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite);
  settings.HandColour = PBL_IF_COLOR_ELSE(GColorYellow, GColorBlack);
  settings.HandOutlineColour = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorBlack);
  settings.BtVibration = true;
  settings.BtBackgroundColour = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
}

static void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_update_display() {
  window_set_background_color(s_window, settings.BackgroundColour);
  layer_mark_dirty(s_clock_layer);
  layer_mark_dirty(s_hands_layer);
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  prv_update_display();
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *bg_colour_t = dict_find(iter, MESSAGE_KEY_BackgroundColour);
  if (bg_colour_t) {
    settings.BackgroundColour = GColorFromHEX(bg_colour_t->value->int32);
  }

  Tuple *ck_colour_t = dict_find(iter, MESSAGE_KEY_ClockColour);
  if (ck_colour_t) {
    settings.ClockColour = GColorFromHEX(ck_colour_t->value->int32);
  }

  Tuple *hd_colour_t = dict_find(iter, MESSAGE_KEY_HandColour);
  if (hd_colour_t) {
    settings.HandColour = GColorFromHEX(hd_colour_t->value->int32);
  }

  Tuple *hdo_colour_t = dict_find(iter, MESSAGE_KEY_HandOutlineColour);
  if (hdo_colour_t) {
    settings.HandOutlineColour = GColorFromHEX(hdo_colour_t->value->int32);
  }

  Tuple *bt_vibe_t = dict_find(iter, MESSAGE_KEY_BtVibration);
  if (bt_vibe_t) {
    settings.BtVibration = bt_vibe_t->value->int32 == 1;
  }

  Tuple *bt_colour_t = dict_find(iter, MESSAGE_KEY_BtBackgroundColour);
  if (bt_colour_t) {
    settings.BtBackgroundColour = GColorFromHEX(bt_colour_t->value->int32);
  }

  prv_save_settings();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void bluetooth_callback(bool connected) {
  window_set_background_color(s_window, settings.BackgroundColour);

  if (!connected) {
    window_set_background_color(s_window, settings.BtBackgroundColour);
   
    if (settings.BtVibration) {
      static const uint32_t const segments[] = { 800, 100, 300 };
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);
    }
  }
}

static void clock_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // White clockface
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint16_t radius = bounds.size.w/2;
  graphics_context_set_fill_color(ctx, settings.ClockColour);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));

  // Background
  gdraw_command_image_draw(ctx, s_bg, GPoint(centre.x-52, centre.y-50));

  // Border
  GRect frame;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(8, 4)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(10, 6)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(64, 48)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(66, 50)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));

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
    int x = (centre.x - (LABELWIDTH/2)) + (sin_lookup(angle) * (radius-PBL_IF_ROUND_ELSE(24,16)) / TRIG_MAX_RATIO);
    int y = (centre.y - (LABELHEIGHT/2)) + (-cos_lookup(angle) * (radius-PBL_IF_ROUND_ELSE(24,16)) / TRIG_MAX_RATIO);
  
    if (i==0) { // red 12
      graphics_context_set_text_color(ctx, GColorRed);
    } else {
      graphics_context_set_text_color(ctx, GColorBlack);
    }

    GRect pos;
    if (i==8) { // offset for 8
      pos = GRect(x+2, y, LABELWIDTH+2, LABELHEIGHT);
    } else {
      pos = GRect(x, y, LABELWIDTH, LABELHEIGHT);
    }

    graphics_draw_text(
      ctx,
      LABELS[i].text,
      s_label_font,
      pos,
      GTextOverflowModeWordWrap,
      GTextAlignmentCenter,
      NULL);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  
  pdc_transform_gdraw_command_image_draw_transformed(
    ctx,
    s_hour_hand,
    PBL_IF_ROUND_ELSE(GPoint(centre.x-10,centre.y-45), GPoint(centre.x-8,centre.y-40)), // origin
    PBL_IF_ROUND_ELSE(12.5, 10), // scale
    (360 * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  
  pdc_transform_gdraw_command_image_draw_transformed(
    ctx,
    s_minute_hand,
    PBL_IF_ROUND_ELSE(GPoint(centre.x-10,centre.y-72), GPoint(centre.x-8,centre.y-60)), // origin
    PBL_IF_ROUND_ELSE(12.5, 10), // scale
    360 * t->tm_min / 60);

  // dot in the middle
  uint32_t radius = bounds.size.w/2; 
  graphics_context_set_fill_color(ctx, settings.HandColour);
  graphics_context_set_stroke_color(ctx, settings.HandOutlineColour);
  GRect frame = grect_inset(bounds, GEdgeInsets(11*radius/12));
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_label_font = fonts_load_custom_font(resource_get_handle(PBL_IF_ROUND_ELSE(RESOURCE_ID_FONT_LABEL_15, RESOURCE_ID_FONT_LABEL_13)));
  
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Clock layer
  s_clock_layer = layer_create(bounds);
  layer_set_update_proc(s_clock_layer, clock_update_proc);
  layer_add_child(window_layer, s_clock_layer);

  // Hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);

  prv_update_display();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_hands_layer);
  layer_destroy(s_clock_layer);
  fonts_unload_custom_font(s_label_font);
  gdraw_command_image_destroy(s_minute_hand);
  gdraw_command_image_destroy(s_hour_hand);
  gdraw_command_image_destroy(s_bg);
}

static void prv_init(void) {
  prv_load_settings();

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  s_minute_hand = gdraw_command_image_create_with_resource(PBL_IF_COLOR_ELSE(RESOURCE_ID_IMAGE_MINUTE_HAND, RESOURCE_ID_IMAGE_MINUTE_HAND_BW));
  s_hour_hand = gdraw_command_image_create_with_resource(PBL_IF_COLOR_ELSE(RESOURCE_ID_IMAGE_HOUR_HAND, RESOURCE_ID_IMAGE_HOUR_HAND_BW));
  s_bg = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_BG);
  
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
