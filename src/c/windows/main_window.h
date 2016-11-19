#pragma once

#include <pebble.h>
#include <pdc-transform/pdc-transform.h>
#include "../modules/settings.h"

#define ABD_LABELHEIGHT 16
#define C_LABELHEIGHT 18
#define E_LABELHEIGHT 20
#define ABD_LABELWIDTH 24
#define C_LABELWIDTH 28
#define E_LABELWIDTH 32

struct Label {
    uint32_t num;
    char text[5];
};

void prv_window_push();
void prv_window_update();
void bluetooth_callback(bool connected);
