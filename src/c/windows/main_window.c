#include "main_window.h"

static Window *s_window;
static Layer *s_window_layer, *s_clock_layer, *s_hands_layer;
static TextLayer *s_digital_layer;
static GDrawCommandImage *s_bg;
static GFont s_label_font, s_label_small_font, s_digital_font;

static void update_digital() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer_time[6];
  strftime(s_buffer_time, sizeof(s_buffer_time), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_digital_layer, s_buffer_time);
}

void prv_window_update() {
  window_set_background_color(s_window, settings.BackgroundColour);
  layer_mark_dirty(s_clock_layer);
  layer_mark_dirty(s_hands_layer);
  if (settings.DigitalQuickView) {
    update_digital();
  }
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  if (grect_equal(&full_bounds, &final_unobstructed_screen_area)) {
    if (settings.DigitalQuickView) {
      layer_set_hidden(text_layer_get_layer(s_digital_layer), true);
    }
  } 
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  if (!grect_equal(&full_bounds, &bounds)) {
    if (settings.DigitalQuickView) {
      layer_set_hidden(text_layer_get_layer(s_digital_layer), false);
    }
  }
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
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(layer);
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;
  
  // White clockface
  if (!obstruction_height == 0) {
    bounds = unobstructed_bounds;
  }
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint16_t radius = bounds.size.h < bounds.size.w ? bounds.size.h/2 : bounds.size.w/2;
  graphics_context_set_fill_color(ctx, settings.ClockColour);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));

  // Background
  if (settings.ShowClockPattern) {
    gdraw_command_image_draw(ctx, s_bg, GPoint(centre.x-52, centre.y-50));
  }

  // Border
  GRect frame;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(8, 4)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(10, 6)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(64, 3*radius/4-2)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(66, 3*radius/4)));
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
    int x = (centre.x - (LABELWIDTH/2)) + (sin_lookup(angle) * (radius-PBL_IF_ROUND_ELSE(radius/3.5, radius/4.5)) / TRIG_MAX_RATIO);
    int y = (centre.y - (LABELHEIGHT/2)) + (-cos_lookup(angle) * (radius-PBL_IF_ROUND_ELSE(radius/3.5, radius/4.5)) / TRIG_MAX_RATIO);
  
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
      obstruction_height > 0 ? s_label_small_font : s_label_font,
      pos,
      GTextOverflowModeWordWrap,
      GTextAlignmentCenter,
      NULL);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  GRect bounds = layer_get_bounds(layer);
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(layer);
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;
  
  // Minute/hour hands
  if (!obstruction_height == 0) {
    bounds = unobstructed_bounds;
  }
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint16_t radius = bounds.size.h < bounds.size.w ? bounds.size.h/2 : bounds.size.w/2;

  GPathInfo HOUR_HAND_INFO = (GPathInfo) {
    .num_points = 9,
    .points = (GPoint[9]) {
/*
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(62,48)},
      {-7, -radius + PBL_IF_ROUND_ELSE(60,46)},
      {-8, -radius + PBL_IF_ROUND_ELSE(54,40)},
      {0, -radius + PBL_IF_ROUND_ELSE(40,26)},
      {8, -radius + PBL_IF_ROUND_ELSE(54,40)},
      {7, -radius + PBL_IF_ROUND_ELSE(60,46)},
      {3, -radius + PBL_IF_ROUND_ELSE(62,48)},
      {3, 4}
*/
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(radius/2 +2, radius/2 +12)},
      {-7, -radius + PBL_IF_ROUND_ELSE(radius/2, radius/2 +10)},
      {-8, -radius + PBL_IF_ROUND_ELSE(radius/2 -6, radius/2 +6)},
      {0, -radius + PBL_IF_ROUND_ELSE(radius/2 -20, radius/2 -10)},
      {8, -radius + PBL_IF_ROUND_ELSE(radius/2 -6, radius/2 +6)},
      {7, -radius + PBL_IF_ROUND_ELSE(radius/2, radius/2 +10)},
      {3, -radius + PBL_IF_ROUND_ELSE(radius/2 +2, radius/2 +12)},
      {3, 4}
    }
  };
  
  GPathInfo MINUTE_HAND_INFO = (GPathInfo) {
    .num_points = 9,
    .points = (GPoint[9]) {
/*
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(42,34)},
      {-7, -radius + PBL_IF_ROUND_ELSE(40,32)},
      {-8, -radius + PBL_IF_ROUND_ELSE(34,26)},
      {0, -radius + PBL_IF_ROUND_ELSE(20,12)},
      {8, -radius + PBL_IF_ROUND_ELSE(34,26)},
      {7, -radius + PBL_IF_ROUND_ELSE(40,32)},
      {3, -radius + PBL_IF_ROUND_ELSE(42,34)},
      {3, 4}
*/
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(radius/3 +2, radius/3 +12)},
      {-7, -radius + PBL_IF_ROUND_ELSE(radius/3, radius/3 +10)},
      {-8, -radius + PBL_IF_ROUND_ELSE(radius/3 -6, radius/3 +6)},
      {0, -radius + PBL_IF_ROUND_ELSE(radius/3 -20, radius/3 -10)},
      {8, -radius + PBL_IF_ROUND_ELSE(radius/3 -6, radius/3 +6)},
      {7, -radius + PBL_IF_ROUND_ELSE(radius/3, radius/3 +10)},
      {3, -radius + PBL_IF_ROUND_ELSE(radius/3 +2, radius/3 +12)},
      {3, 4}
    }
  };

  GPath *s_h_hand = gpath_create(&HOUR_HAND_INFO);
  GPath *s_m_hand = gpath_create(&MINUTE_HAND_INFO);
  
  gpath_move_to(s_h_hand, centre);
  gpath_move_to(s_m_hand, centre);
  gpath_rotate_to(s_h_hand, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_rotate_to(s_m_hand, TRIG_MAX_ANGLE * t->tm_min / 60);

  graphics_context_set_fill_color(ctx, settings.HandColour);
  graphics_context_set_stroke_color(ctx, settings.HandOutlineColour);

  if (settings.HourOverMinute) {
    gpath_draw_filled(ctx, s_m_hand);
    gpath_draw_outline(ctx, s_m_hand);
    gpath_draw_filled(ctx, s_h_hand);
    gpath_draw_outline(ctx, s_h_hand);
  } else {
    gpath_draw_filled(ctx, s_h_hand);
    gpath_draw_outline(ctx, s_h_hand);
    gpath_draw_filled(ctx, s_m_hand);
    gpath_draw_outline(ctx, s_m_hand);
  }

  gpath_destroy(s_h_hand);
  gpath_destroy(s_m_hand);

  // Dot in the middle
  GRect frame = grect_inset(bounds, GEdgeInsets(11*radius/12));
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
}

static void prv_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  s_label_font = fonts_load_custom_font(resource_get_handle(PBL_IF_ROUND_ELSE(RESOURCE_ID_FONT_LABEL_14, RESOURCE_ID_FONT_LABEL_13)));
  s_label_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LABEL_12));

  if (settings.ShowClockPattern) {
    s_bg = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_BG);
  }

  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Clock layer
  s_clock_layer = layer_create(bounds);
  layer_set_update_proc(s_clock_layer, clock_update_proc);
  layer_add_child(s_window_layer, s_clock_layer);

  // Hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(s_window_layer, s_hands_layer);

  // Digital quick view layer
  if (settings.DigitalQuickView) {
    s_digital_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LABEL_24));
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    GRect digital_bounds = GRect(0,
                                 bounds.size.h/2 + (DIGITALHEIGHT/4),
                                 bounds.size.w,
                                 DIGITALHEIGHT
    );
    s_digital_layer = text_layer_create(digital_bounds);
    text_layer_set_text_alignment(s_digital_layer, GTextAlignmentCenter);
    text_layer_set_background_color(s_digital_layer, GColorClear);
    text_layer_set_text_color(s_digital_layer, settings.DigitalQuickViewColour);
    text_layer_set_font(s_digital_layer, s_digital_font);
    layer_add_child(s_window_layer, text_layer_get_layer(s_digital_layer));

    if (grect_equal(&bounds, &unobstructed_bounds)) {
      layer_set_hidden(text_layer_get_layer(s_digital_layer), true);
    }
  }

  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  prv_window_update();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_hands_layer);
  layer_destroy(s_clock_layer);
  if (settings.DigitalQuickView) {
    text_layer_destroy(s_digital_layer);
    fonts_unload_custom_font(s_digital_font);
  }
  fonts_unload_custom_font(s_label_font);
  fonts_unload_custom_font(s_label_small_font);
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
