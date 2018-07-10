#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer,
                 *s_date_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_bitmap_led;
static int s_battery_level;

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
  strftime(s_buffer, sizeof(s_buffer), "%b %e", tick_time);
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

  graphics_context_set_fill_color(ctx, GColorElectricBlue);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void bluetooth_callback(bool connected) {
  if (!connected) {   
    static const uint32_t const segments[] = { 200, 200, 200, 200, 200 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);
  }
}


static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_background_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  s_bitmap_led = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LED_B);
  bitmap_layer_set_bitmap(s_background_layer, s_bitmap_led);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  s_time_layer = text_layer_create(GRect(0, bounds.size.h-40, bounds.size.w-4, 40));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  update_time();
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(GRect(4, bounds.size.h-24, bounds.size.w, 24));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  update_date();
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_battery_layer = layer_create(GRect(4, bounds.size.h-28, bounds.size.w/4, 4));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);
  battery_callback(battery_state_service_peek());

  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);
}

static void prv_init(void) {
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  gbitmap_destroy(s_bitmap_led);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_background_layer);
  window_destroy(s_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
