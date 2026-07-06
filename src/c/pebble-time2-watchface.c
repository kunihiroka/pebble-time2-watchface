#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_steps_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time(struct tm *tick_time) {
  static char s_time_buffer[8];
  static char s_date_buffer[24];

  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%Y-%m-%d (%a)", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void update_battery(BatteryChargeState charge_state) {
  static char s_battery_buffer[8];

  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%",
           charge_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_steps(void) {
  static char s_steps_buffer[16];

  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", (int)steps);
  text_layer_set_text(s_steps_layer, s_steps_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  update_steps();
}

static void battery_handler(BatteryChargeState charge_state) {
  update_battery(charge_state);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    update_steps();
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  s_time_layer = text_layer_create(GRect(0, 20, 130, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer,
                       fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_battery_layer = text_layer_create(GRect(135, 28, 60, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_font(s_battery_layer,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  s_steps_layer = text_layer_create(GRect(135, 52, 60, 20));
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_color(s_steps_layer, GColorWhite);
  text_layer_set_font(s_steps_layer,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));

  s_date_layer = text_layer_create(
      GRect(0, 80, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  update_time(tick_time);
  update_battery(battery_state_service_peek());
  update_steps();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_steps_layer);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  health_service_events_subscribe(health_handler, NULL);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
