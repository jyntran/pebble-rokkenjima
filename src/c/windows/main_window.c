#include "main_window.h"

static Window *s_window;
static Layer *s_window_layer, *s_clock_layer, *s_hands_layer;
static GDrawCommandImage *s_bg;
static GFont s_label_font, s_label_small_font;
static GPath *s_h_hand, *s_m_hand;

static GPathInfo HOUR_HAND_INFO;
static GPathInfo MINUTE_HAND_INFO;

static GPoint centre;
static uint16_t radius;
static GPoint origin;
static uint16_t scale = 10;

static uint16_t LABELHEIGHT = 0;
static uint16_t LABELWIDTH = 0;
static uint16_t LABELDISTANCE = 0;

static void set_variables() {
  switch (PBL_PLATFORM_TYPE_CURRENT) {
    case PlatformTypeChalk:
      s_label_font = fonts_load_custom_font(resource_get_handle(C_LABEL_FONT));
      s_label_small_font = fonts_load_custom_font(resource_get_handle(C_LABEL_SMALL_FONT));
      LABELHEIGHT = C_LABELHEIGHT;
      LABELWIDTH = C_LABELWIDTH;
      break;
    case PlatformTypeEmery:
      s_label_font = fonts_load_custom_font(resource_get_handle(E_LABEL_FONT));
      s_label_small_font = fonts_load_custom_font(resource_get_handle(E_LABEL_SMALL_FONT));
      LABELHEIGHT = E_LABELHEIGHT;
      LABELWIDTH = E_LABELWIDTH;
      break;
    default:
      s_label_font = fonts_load_custom_font(resource_get_handle(ABD_LABEL_FONT));
      s_label_small_font = fonts_load_custom_font(resource_get_handle(ABD_LABEL_SMALL_FONT));
      LABELHEIGHT = ABD_LABELHEIGHT;
      LABELWIDTH = ABD_LABELWIDTH;
  }
}

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
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(layer);
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;
  
  // White clockface
  if (!obstruction_height == 0) {
    bounds = unobstructed_bounds;
  }
  centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  radius = bounds.size.h < bounds.size.w ? bounds.size.h/2 : bounds.size.w/2;
  graphics_context_set_fill_color(ctx, settings.ClockColour);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));

  // Background
  if (settings.ShowClockPattern) {
    if (obstruction_height == 0) {
    switch (PBL_PLATFORM_TYPE_CURRENT) {
      case PlatformTypeChalk:
        origin = GPoint(centre.x-62, centre.y-60);
        scale = 12;
        break;
      case PlatformTypeEmery:
        origin = GPoint(centre.x-72, centre.y-70);
        scale = 14;
        break;
      default:
        origin = GPoint(centre.x-52, centre.y-50);
        scale = 10;
    }
    } else {
    switch (PBL_PLATFORM_TYPE_CURRENT) {
      case PlatformTypeChalk:
        origin = GPoint(centre.x-52, centre.y-50);
        scale = 10;
        break;
      case PlatformTypeEmery:
        origin = GPoint(centre.x-62, centre.y-60);
        scale = 12;
        break;
      default:
        origin = GPoint(centre.x-40, centre.y-40);
        scale = 8;
    }
    }

    if (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeAplite) {
      gdraw_command_image_draw(ctx, s_bg, origin);
    } else {
      pdc_transform_gdraw_command_image_draw_transformed(ctx, s_bg, origin, scale, 0);
    }
  }

  // Border
  GRect frame;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(6, 4)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(8, 6)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(2*radius/3, 2*radius/3-2)));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
  frame = grect_inset(bounds, GEdgeInsets(PBL_IF_ROUND_ELSE(2*radius/3+2, 2*radius/3)));
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
  LABELDISTANCE = radius-radius/4;

  for (int i=0; i <12; i++) {
    int angle = (TRIG_MAX_ANGLE * (i % 12) * 6) / (12 * 6);
    int x = (centre.x - (LABELWIDTH/2)) + (sin_lookup(angle) * LABELDISTANCE / TRIG_MAX_RATIO);
    int y = (centre.y - (LABELHEIGHT/2)) + (-cos_lookup(angle) * LABELDISTANCE / TRIG_MAX_RATIO);
  
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
  centre = GPoint(bounds.size.w/2, bounds.size.h/2);
  radius = bounds.size.h < bounds.size.w ? bounds.size.h/2 : bounds.size.w/2;

  HOUR_HAND_INFO = (GPathInfo) {
    9,
    (GPoint[9]) {
      {-3, 4},
      {-3, -radius + radius/2 +12},
      {-7, -radius + radius/2 +10},
      {-8, -radius + radius/2 +6},
      {0, -radius + radius/2 -10},
      {8, -radius + radius/2 +6},
      {7, -radius + radius/2 +10},
      {3, -radius + radius/2 +12},
      {3, 4}
    }
  };

  MINUTE_HAND_INFO = (GPathInfo) {
    9,
    (GPoint[9]) {
      {-3, 4},
      {-3, -radius + radius/3 +12},
      {-7, -radius + radius/3 +10},
      {-8, -radius + radius/3 +6},
      {0, -radius + radius/3 -10},
      {8, -radius + radius/3 +6},
      {7, -radius + radius/3 +10},
      {3, -radius + radius/3 +12},
      {3, 4}
    }
  };

  if (s_h_hand) { gpath_destroy(s_h_hand); }
  if (s_m_hand) { gpath_destroy(s_m_hand); }

  s_h_hand = gpath_create(&HOUR_HAND_INFO);
  s_m_hand = gpath_create(&MINUTE_HAND_INFO);

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

  // Dot in the middle
  GRect frame = grect_inset(bounds, GEdgeInsets(11*radius/12));
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, radius, 0, DEG_TO_TRIGANGLE(360));
  graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, 0, DEG_TO_TRIGANGLE(360));
}

static void prv_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  if (settings.ShowClockPattern) {
    switch (settings.ClockPatternColour) {
      case 1:
        s_bg = gdraw_command_image_create_with_resource(BG_RED);
        break;
      default:
        s_bg = gdraw_command_image_create_with_resource(BG_GRAY);
    }
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

  prv_window_update();
}

static void prv_window_unload(Window *window) {
  fonts_unload_custom_font(s_label_font);
  fonts_unload_custom_font(s_label_small_font);
  gdraw_command_image_destroy(s_bg);
  gpath_destroy(s_h_hand);
  gpath_destroy(s_m_hand);
  layer_destroy(s_hands_layer);
  layer_destroy(s_clock_layer);
  //if (s_window_layer) { layer_destroy(s_window_layer); }
  if (s_window) { window_destroy(s_window); }
}

void prv_window_push() {
  set_variables();
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
  }
  window_stack_push(s_window, true);
}
