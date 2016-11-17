#include <pebble.h>
#include "rokkenjima.h"
#include "modules/settings.h"
#include "windows/main_window.h"

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & HOUR_UNIT) {
    static const uint32_t const segments[] = { 400, 100, 150 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);
  }

  prv_window_update();
}

static void prv_init(void) {
  prv_load_settings();

  prv_window_push();

  if (settings.HourlyVibration) {
    tick_timer_service_subscribe(HOUR_UNIT + MINUTE_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
