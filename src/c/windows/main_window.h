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

#define BG_GRAY RESOURCE_ID_IMAGE_BG_GRAY
#define BG_RED RESOURCE_ID_IMAGE_BG_RED
#define BG_BLUE RESOURCE_ID_IMAGE_BG_BLUE
#define BG_GREEN RESOURCE_ID_IMAGE_BG_GREEN
#define BG_YELLOW RESOURCE_ID_IMAGE_BG_YELLOW
#define BG_GOLD RESOURCE_ID_IMAGE_BG_GOLD
#define BG_PINK RESOURCE_ID_IMAGE_BG_PINK

struct Label {
    uint32_t num;
    char text[5];
};

void prv_window_push();
void prv_window_update();
void bluetooth_callback(bool connected);
