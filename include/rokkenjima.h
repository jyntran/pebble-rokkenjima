#pragma once

#include "pebble.h"

#define LABELHEIGHT 16
#define LABELWIDTH 28

struct Label {
    uint32_t num;
    char text[5];
};
