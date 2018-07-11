#include <pebble.h>

static Window      *s_window;
static TextLayer   *s_time_layer,
                   *s_date_layer,
                   *s_batt_layer,
                   *s_health_layer;
static Layer       *s_battery_layer;
static BitmapLayer *s_led_blue_layer,
                   *s_led_red_layer;
static GBitmap     *s_bitmap_led_blue,
                   *s_bitmap_led_red;
static int          s_battery_level;

#if defined(PBL_HEALTH)
  static int        s_health_count;
#endif

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_buffer);
}

static void update_date() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer[16];
  strftime(s_buffer, sizeof(s_buffer), "%b %e\n%Y", tick_time);
  text_layer_set_text(s_date_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  if (units_changed & DAY_UNIT) {
    update_date();
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  int width = (s_battery_level * (bounds.size.w)) / 100;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, bounds.size.h/2, bounds.size.w, bounds.size.h/2), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);

  static char s_buffer[8];
  snprintf(s_buffer, sizeof(s_buffer), "%d%%", s_battery_level);
  text_layer_set_text(s_batt_layer, s_buffer);
}

static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_led_red_layer), connected);
  if (!connected) {
    static const uint32_t const segments[] = { 200, 200, 200, 200, 200 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);
  }
}

#if defined(PBL_HEALTH)
  static void update_health() {
    HealthMetric metric = HealthMetricWalkedDistanceMeters;
    time_t start = time_start_of_today();
    time_t end = time(NULL);

    HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
      start, end);

    if (mask & HealthServiceAccessibilityMaskAvailable) {
      s_health_count = (int) health_service_sum_today(metric);
      static char s_buffer[32];
      snprintf(s_buffer, sizeof(s_buffer), "%dm", s_health_count);    
      text_layer_set_text(s_health_layer, s_buffer);
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
    }
  }

  static void health_callback(HealthEventType event, void *context) {
    switch(event) {
      case HealthEventMovementUpdate:
        update_health();
        break;
      default:
        break;
    }
  }
#endif

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(s_window, GColorBlack);

  s_led_blue_layer = bitmap_layer_create(GRect(0, 2, bounds.size.w, bounds.size.w-2));
  bitmap_layer_set_compositing_mode(s_led_blue_layer, GCompOpSet);
  s_bitmap_led_blue = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LED);
  bitmap_layer_set_bitmap(s_led_blue_layer, s_bitmap_led_blue);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_led_blue_layer));

  s_led_red_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w, bounds.size.w));
  bitmap_layer_set_compositing_mode(s_led_red_layer, GCompOpSet);
  s_bitmap_led_red = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LED_RED);
  bitmap_layer_set_bitmap(s_led_red_layer, s_bitmap_led_red);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_led_red_layer));

  s_time_layer = text_layer_create(GRect(0, bounds.size.h-40, bounds.size.w-4, 40));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  update_time();
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(GRect(4, bounds.size.h-32, bounds.size.w, 32));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  update_date();
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_batt_layer = text_layer_create(GRect(4, 4, bounds.size.w, 24));
  text_layer_set_font(s_batt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_batt_layer, GTextAlignmentLeft);
  text_layer_set_background_color(s_batt_layer, GColorClear);
  text_layer_set_text_color(s_batt_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_batt_layer));

  s_battery_layer = layer_create(GRect(4, 4, bounds.size.w/2, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);
  battery_callback(battery_state_service_peek());

  bluetooth_callback(connection_service_peek_pebble_app_connection());

  #if defined(PBL_HEALTH)
    s_health_layer = text_layer_create(GRect(bounds.size.w/2, 0, bounds.size.w/2-4, 24));
    text_layer_set_font(s_health_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(s_health_layer, GTextAlignmentRight);
    text_layer_set_background_color(s_health_layer, GColorClear);
    text_layer_set_text_color(s_health_layer, GColorWhite);
    update_health();
    layer_add_child(window_layer, text_layer_get_layer(s_health_layer));
  #endif
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_batt_layer);
  text_layer_destroy(s_health_layer);
  layer_destroy(s_battery_layer);
}

static void prv_init(void) {
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  #if defined(PBL_HEALTH)
  if (!health_service_events_subscribe(health_callback, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }
  #endif

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  gbitmap_destroy(s_bitmap_led_red);
  gbitmap_destroy(s_bitmap_led_blue);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_batt_layer);
  text_layer_destroy(s_health_layer);
  bitmap_layer_destroy(s_led_blue_layer);
  window_destroy(s_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  health_service_events_unsubscribe();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
