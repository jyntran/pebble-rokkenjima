#include "main_window.h"

static Window *s_window;
static Layer *s_clock_layer, *s_hands_layer;
static GDrawCommandImage *s_bg;
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

  // minute/hour hands
  GPoint centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  uint32_t radius = bounds.size.w/2; 

  GPathInfo HOUR_HAND_INFO = (GPathInfo) {
    .num_points = 9,
    .points = (GPoint[9]) {
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(62,48)},
      {-7, -radius + PBL_IF_ROUND_ELSE(60,46)},
      {-8, -radius + PBL_IF_ROUND_ELSE(54,40)},
      {0, -radius + PBL_IF_ROUND_ELSE(42,28)},
      {8, -radius + PBL_IF_ROUND_ELSE(54,40)},
      {7, -radius + PBL_IF_ROUND_ELSE(60,46)},
      {3, -radius + PBL_IF_ROUND_ELSE(62,48)},
      {3, 4}
    }
  };
  
  GPathInfo MINUTE_HAND_INFO = (GPathInfo) {
    .num_points = 9,
    .points = (GPoint[9]) {
      {-3, 4},
      {-3, -radius + PBL_IF_ROUND_ELSE(42,34)},
      {-7, -radius + PBL_IF_ROUND_ELSE(40,32)},
      {-8, -radius + PBL_IF_ROUND_ELSE(34,26)},
      {0, -radius + PBL_IF_ROUND_ELSE(22,14)},
      {8, -radius + PBL_IF_ROUND_ELSE(34,26)},
      {7, -radius + PBL_IF_ROUND_ELSE(40,32)},
      {3, -radius + PBL_IF_ROUND_ELSE(42,34)},
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
  gpath_draw_filled(ctx, s_h_hand);
  gpath_draw_filled(ctx, s_m_hand);

  graphics_context_set_stroke_color(ctx, settings.HandOutlineColour);
  gpath_draw_outline(ctx, s_h_hand);
  gpath_draw_outline(ctx, s_m_hand);

  gpath_destroy(s_h_hand);
  gpath_destroy(s_m_hand);

  // dot in the middle
  GRect frame = grect_inset(bounds, GEdgeInsets(11*radius/12));
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_label_font = fonts_load_custom_font(resource_get_handle(PBL_IF_ROUND_ELSE(RESOURCE_ID_FONT_LABEL_15, RESOURCE_ID_FONT_LABEL_13)));

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
