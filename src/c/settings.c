#include "settings.h"

static void prv_default_settings() {
  settings.ShowBatteryPercentage = true;
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *hvb_bool_t = dict_find(iter, MESSAGE_KEY_ShowBatteryPercentage);
    if (hvb_bool_t) {
      settings.ShowBatteryPercentage = hvb_bool_t->value->int32 == 1;
  }
  
  prv_save_settings();
}

void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}
