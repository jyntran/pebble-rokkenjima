#pragma once

#include <pebble.h>
#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColour;
  GColor ClockColour;
  bool ShowClockPattern;
  uint32_t ClockPatternColour;
  GColor HandColour;
  GColor HandOutlineColour;
  bool HourOverMinute;
  bool HourlyVibration;
  bool BtVibration;
  GColor BtBackgroundColour;
} ClaySettings;

struct ClaySettings settings;

void prv_load_settings();
