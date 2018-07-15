#include "settings.h"

static void prv_default_settings() {
  settings.ShowBatteryPercentage = true;
  settings.ShowDailyMetres = true;
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *battery_tuple = dict_find(iter, MESSAGE_KEY_ShowBatteryPercentage);
  if (battery_tuple) {
    settings.ShowBatteryPercentage = battery_tuple->value->int32 == 1;
  }

  Tuple *daily_metres_tuple = dict_find(iter, MESSAGE_KEY_ShowDailyMetres);
  if (daily_metres_tuple) {
    settings.ShowDailyMetres = daily_metres_tuple->value->int32 == 1;
  }
  
  Tuple *temp_tuple = dict_find(iter, MESSAGE_KEY_WeatherTemperature);
  if (temp_tuple) {
    settings.WeatherTemperature = (int)temp_tuple->value->int32;
  }

  prv_save_settings();
}

void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}
