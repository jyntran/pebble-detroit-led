#pragma once

#include <pebble.h>
#define SETTINGS_KEY 1

typedef struct ClaySettings {
  bool ShowBatteryPercentage;
  bool ShowDailyMetres;
  int WeatherTemperature;
} ClaySettings;

struct ClaySettings settings;

void prv_load_settings();