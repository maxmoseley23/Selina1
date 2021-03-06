#include <pebble.h>
#include "fctx/fctx.h"
#include "fctx/ffont.h"


Window *my_window;

static TextLayer *battery;
static Layer *time_layer, *date_layer;
FFont* time_font;
FFont* bold_font;
FFont* reg_font;


static char date_buffer[] = "31 Octubre";
static char time_buffer[] = "23:55";
static char wd_buffer[] = "Wednesday";
static char b_buffer[] = "100% - OK";

static void battery_handler(BatteryChargeState charge);

void date_layer_update(Layer* layer, GContext* ctx) {

  GRect bounds = layer_get_bounds(layer);
 FPoint weekday_pos =  PBL_IF_ROUND_ELSE(
      FPointI(bounds.size.w / 2, 1*(bounds.size.h/2) + 44),
      FPointI(bounds.size.w / 2, 1*(bounds.size.h/2) + 40)
    );

 FPoint date_pos =  PBL_IF_ROUND_ELSE(
    FPointI(bounds.size.w / 2, 1*(bounds.size.h/2) - 42),
    FPointI(bounds.size.w / 2, 1*(bounds.size.h/2) - 41)
    );




 int weekday_font_size = 18;
 int date_font_size = 20;
  FContext fctx;
  fctx_init_context(&fctx, ctx);

  fctx_begin_fill(&fctx);
  fctx_set_offset(&fctx, weekday_pos);
  fctx_set_text_em_height(&fctx, reg_font, weekday_font_size);
  fctx_set_fill_color(&fctx, GColorBlack);
  fctx_draw_string(&fctx, wd_buffer, reg_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);

  fctx_begin_fill(&fctx);
  fctx_set_offset(&fctx, date_pos);
  fctx_set_text_em_height(&fctx, bold_font, date_font_size);
  fctx_set_fill_color(&fctx, PBL_IF_COLOR_ELSE(GColorTiffanyBlue, GColorBlack));
  fctx_draw_string(&fctx, date_buffer, bold_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);


  fctx_deinit_context(&fctx);
}

void time_layer_update(Layer* layer, GContext* ctx) {


  GRect bounds = layer_get_bounds(layer);
 FPoint center = FPointI(bounds.size.w / 2, 
  (bounds.size.h / 2) - 2
  );

  int font_size;

  #if defined(PBL_ROUND)
    font_size = 54;
  #else
    font_size = 49;
  #endif

  FContext fctx;
  fctx_init_context(&fctx, ctx);

  fctx_begin_fill(&fctx);
  fctx_set_offset(&fctx, center);
  fctx_set_text_em_height(&fctx, time_font, font_size);
  fctx_set_fill_color(&fctx, GColorBlack);
  fctx_draw_string(&fctx, time_buffer+((' ' == time_buffer[0])?1:0), time_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);


  fctx_deinit_context(&fctx);


}

static void tick_handler(struct tm *t, TimeUnits changed) {

  if (MINUTE_UNIT & changed) {
    strftime(time_buffer, sizeof(time_buffer), 
    clock_is_24h_style() ?"%k:%M" : "%Il%M", t);
    layer_mark_dirty(time_layer);
  }

  if (DAY_UNIT & changed) {
    char month[]="Decembre";
    strftime(wd_buffer, sizeof(wd_buffer), "%A", t);
    strftime(month, sizeof(month), "%b", t);
    snprintf(date_buffer, sizeof(date_buffer), "%d %s", t->tm_mday, month);
    layer_mark_dirty(date_layer);
  }
}

static void main_window_load(Window *window) {
  char *sys_locale = setlocale(LC_ALL, "");
  
  window_set_background_color(my_window, GColorWhite);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  time_font = ffont_create_from_resource(RESOURCE_ID_TIME_FONT);
  bold_font = ffont_create_from_resource(RESOURCE_ID_BOLD_FONT);
  reg_font = ffont_create_from_resource(RESOURCE_ID_REG_FONT);

  time_layer = layer_create(PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) - 32, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) - 32, bounds.size.w, bounds.size.h)
    ));
  layer_set_update_proc(time_layer, time_layer_update);
  layer_add_child(window_layer, time_layer);

  
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


  date_layer = layer_create(
    PBL_IF_ROUND_ELSE(
      GRect(0, 1*(bounds.size.h/2) + 32, bounds.size.w, bounds.size.h),
      GRect(0, 1*(bounds.size.h/2) + 29, bounds.size.w, bounds.size.h)
    ));
  layer_set_update_proc(date_layer, date_layer_update);
  layer_add_child(window_layer, date_layer);
  
  
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

  if (connection_service_peek_pebble_app_connection()) {
    snprintf(b_buffer, sizeof(b_buffer), "%d%% - OK", charge.charge_percent);
//     snprintf(b_buffer, sizeof(b_buffer), "%d%%", charge.charge_percent);
  } else {
    snprintf(b_buffer, sizeof(b_buffer), "%d%%", charge.charge_percent);
  }
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
  battery_handler(battery_state_service_peek());
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
  ffont_destroy(time_font);
  ffont_destroy(bold_font);
  ffont_destroy(reg_font);
  layer_destroy(time_layer);
  layer_destroy(date_layer);
  text_layer_destroy(battery);
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
	handle_init();
  app_event_loop();
  handle_deinit();
}
