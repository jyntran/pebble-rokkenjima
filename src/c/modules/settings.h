#pragma once

#include <pebble.h>
#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColour;
  GColor ClockColour;
  bool ShowClockPattern;
  GColor HandColour;
  GColor HandOutlineColour;
  bool HourOverMinute;
  bool BtVibration;
  GColor BtBackgroundColour;
} ClaySettings;

struct ClaySettings settings;

void prv_load_settings();