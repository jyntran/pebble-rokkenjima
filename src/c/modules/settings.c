#include "settings.h"

struct ClaySettings settings;

static void prv_default_settings() {
  settings.BackgroundColour = GColorBlack;
  settings.ClockColour = PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite);
  settings.HandColour = PBL_IF_COLOR_ELSE(GColorYellow, GColorBlack);
  settings.HandOutlineColour = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite);
  settings.BtVibration = true;
  settings.BtBackgroundColour = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  //prv_window_update();
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *bg_colour_t = dict_find(iter, MESSAGE_KEY_BackgroundColour);
  if (bg_colour_t) {
    settings.BackgroundColour = GColorFromHEX(bg_colour_t->value->int32);
  }

  Tuple *ck_colour_t = dict_find(iter, MESSAGE_KEY_ClockColour);
  if (ck_colour_t) {
    settings.ClockColour = GColorFromHEX(ck_colour_t->value->int32);
  }

  Tuple *hd_colour_t = dict_find(iter, MESSAGE_KEY_HandColour);
  if (hd_colour_t) {
    settings.HandColour = GColorFromHEX(hd_colour_t->value->int32);
  }

  Tuple *hdo_colour_t = dict_find(iter, MESSAGE_KEY_HandOutlineColour);
  if (hdo_colour_t) {
    settings.HandOutlineColour = GColorFromHEX(hdo_colour_t->value->int32);
  }

  Tuple *bt_vibe_t = dict_find(iter, MESSAGE_KEY_BtVibration);
  if (bt_vibe_t) {
    settings.BtVibration = bt_vibe_t->value->int32 == 1;
  }

  Tuple *bt_colour_t = dict_find(iter, MESSAGE_KEY_BtBackgroundColour);
  if (bt_colour_t) {
    settings.BtBackgroundColour = GColorFromHEX(bt_colour_t->value->int32);
  }

  prv_save_settings();
}

void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}

