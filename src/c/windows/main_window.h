#pragma once

#include <pebble.h>
#include <pdc-transform/pdc-transform.h>
#include "../modules/settings.h"

#define LABELHEIGHT 16
#define LABELWIDTH 24

struct Label {
    uint32_t num;
    char text[5];
};

void prv_window_push();
void prv_window_update();
void bluetooth_callback(bool connected);
