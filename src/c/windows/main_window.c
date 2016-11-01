#include "main_window.h"

static Window *s_window;
static Layer *s_clock_layer, *s_hands_layer;
static GDrawCommandImage *s_bg, *s_minute_hand, *s_hour_hand;
static GFont s_label_font;

void prv_window_update() {
  window_set_background_color(s_window, settings.BackgroundColour);
  layer_mark_dirty(s_clock_layer);
  layer_mark_dirty(s_hands_layer);
}

void bluetooth_callback(bool connected) {
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

  s_minute_hand = gdraw_command_image_create_with_resource(PBL_IF_COLOR_ELSE(RESOURCE_ID_IMAGE_MINUTE_HAND, RESOURCE_ID_IMAGE_MINUTE_HAND_BW));
  s_hour_hand = gdraw_command_image_create_with_resource(PBL_IF_COLOR_ELSE(RESOURCE_ID_IMAGE_HOUR_HAND, RESOURCE_ID_IMAGE_HOUR_HAND_BW));
  s_bg = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_BG);

  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Clock layer
  s_clock_layer = layer_create(bounds);
  layer_set_update_proc(s_clock_layer, clock_update_proc);
  layer_add_child(window_layer, s_clock_layer);

  // Hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);

  prv_window_update();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_hands_layer);
  layer_destroy(s_clock_layer);
  fonts_unload_custom_font(s_label_font);
  gdraw_command_image_destroy(s_minute_hand);
  gdraw_command_image_destroy(s_hour_hand);
  gdraw_command_image_destroy(s_bg);
  if (s_window) { window_destroy(s_window); }
}

void prv_window_push() {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
  }
  window_stack_push(s_window, true);
}
