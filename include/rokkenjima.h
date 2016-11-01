#pragma once

#include "pebble.h"

#define LABELHEIGHT 16
#define LABELWIDTH 24

struct Label {
    uint32_t num;
    char text[5];
};

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColour;
  GColor ClockColour;
  GColor HandColour;
  GColor HandOutlineColour;
  bool BtVibration;
  GColor BtBackgroundColour;
} ClaySettings;
