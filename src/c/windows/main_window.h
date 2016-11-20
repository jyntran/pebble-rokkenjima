#pragma once

#include <pebble.h>
#include <pdc-transform/pdc-transform.h>
#include "../modules/settings.h"

#define ABD_LABEL_FONT RESOURCE_ID_FONT_LABEL_13
#define ABD_LABEL_SMALL_FONT RESOURCE_ID_FONT_LABEL_12
#define C_LABEL_FONT RESOURCE_ID_FONT_LABEL_16
#define C_LABEL_SMALL_FONT RESOURCE_ID_FONT_LABEL_13
#define E_LABEL_FONT RESOURCE_ID_FONT_LABEL_20
#define E_LABEL_SMALL_FONT RESOURCE_ID_FONT_LABEL_18

#define ABD_LABELHEIGHT 16
#define C_LABELHEIGHT 20
#define E_LABELHEIGHT 24
#define ABD_LABELWIDTH 24
#define C_LABELWIDTH 28
#define E_LABELWIDTH 34

struct Label {
    uint32_t num;
    char text[5];
};

void prv_window_push();
void prv_window_update();
void bluetooth_callback(bool connected);
