#include <pebble.h>
Window *my_window;

static TextLayer *time_layer, *date, *weekday, *battery;


static char date_buffer[] = "31 Oct";
static char time_buffer[] = "23:55";
static char wd_buffer[] = "Wednesday";
static char b_buffer[] = "100%";

static void battery_handler(BatteryChargeState charge);



static void tick_handler(struct tm *t, TimeUnits changed) {

  if (MINUTE_UNIT & changed) {
    strftime(time_buffer, sizeof(time_buffer), 
    clock_is_24h_style() ?"%k:%M" : "%l:%M", t);
    text_layer_set_text(time_layer, time_buffer);
  }

  if (DAY_UNIT & changed) {
    char month[]="Dec";
    strftime(wd_buffer, sizeof(wd_buffer), "%A", t);
    strftime(month, sizeof(month), "%b", t);
    snprintf(date_buffer, sizeof(date_buffer), "%d %s.", t->tm_mday, month);
    text_layer_set_text(date, date_buffer);
    text_layer_set_text(weekday, wd_buffer);
  }
}

static void main_window_load(Window *window) {
  
  window_set_background_color(my_window, GColorWhite);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  time_layer = text_layer_create(
    PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) - 34, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) - 32, bounds.size.w, bounds.size.h)
    ));
  text_layer_set_font(time_layer, 
                      PBL_IF_ROUND_ELSE(
                        fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIME_54)),
                        fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49)
                      )
   );
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text(time_layer, "23:59");
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  date = text_layer_create(
    PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) - 55, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) - 53, bounds.size.w, bounds.size.h)
    ));
  text_layer_set_font(date, fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_BOLD_20)));
  text_layer_set_background_color(date, GColorClear);
  text_layer_set_text_color(date, PBL_IF_COLOR_ELSE(GColorTiffanyBlue, GColorBlack));
  text_layer_set_text_alignment(date, GTextAlignmentCenter);
  text_layer_set_text(date, "4 Oct");
  layer_add_child(window_layer, text_layer_get_layer(date));
  
  battery = text_layer_create(
    PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) - 81, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) - 80, bounds.size.w, bounds.size.h)
    ));
  text_layer_set_font(battery, fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_LIGHT_12)));
  text_layer_set_background_color(battery, GColorClear);
  text_layer_set_text_color(battery, GColorBlack);
  text_layer_set_text_alignment(battery, GTextAlignmentCenter);
//   text_layer_set_text(battery, "100%");
  layer_add_child(window_layer, text_layer_get_layer(battery));
  
  weekday = text_layer_create(
    PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) + 32, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) + 29, bounds.size.w, bounds.size.h)
    ));
  text_layer_set_font(weekday, fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_REG_18)));
  text_layer_set_background_color(weekday, GColorClear);
  text_layer_set_text_color(weekday, GColorDarkGray);
  text_layer_set_text_alignment(weekday, GTextAlignmentCenter);
  text_layer_set_text(weekday, "Wednesday");
  layer_add_child(window_layer, text_layer_get_layer(weekday));
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  tick_handler(tick_time, DAY_UNIT | MINUTE_UNIT); // initialize
  battery_handler(battery_state_service_peek());
}

static void battery_handler(BatteryChargeState charge) {
  
  #if defined(PBL_COLOR)
    if (charge.charge_percent == 100 && charge.charge_percent > 50) {
      text_layer_set_text_color(battery, GColorBlack);
    } else if (charge.charge_percent < 25 ) {
      text_layer_set_text_color(battery, GColorRed);
    } else if (charge.charge_percent <= 50 && charge.charge_percent >= 25) {
      text_layer_set_text_color(battery, GColorChromeYellow);
    }
  #else 
    text_layer_set_text_color(battery, GColorBlack);
  #endif

  
  snprintf(b_buffer, sizeof(b_buffer), "%d%%", charge.charge_percent);
  text_layer_set_text(battery, b_buffer);
  
}

static void app_connection_handler(bool connected) {
  if (! connected) {
      static const uint32_t const segments[] = 
        { 1000, 500, 400, 300, 200 };
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);
  }
}

void handle_init(void) {


  my_window = window_create();

  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
  });

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = app_connection_handler,
  });
  
  battery_state_service_subscribe(battery_handler);

  window_stack_push(my_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);
}

void handle_deinit(void) {

  window_destroy(my_window);
  text_layer_destroy(time_layer);
  text_layer_destroy(weekday);
  text_layer_destroy(battery);
  text_layer_destroy(date);
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
	handle_init();
  app_event_loop();
  handle_deinit();
}
