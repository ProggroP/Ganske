#include "pebble.h"

static Window *s_window;
static Layer *s_hands_layer, *s_dot_layer;


typedef struct ClaySettings {
  GColor color;
  int    inverted;
  int    thicker;
  int    twentyfour;
  int    bigonly;
  int    smooth;
  int    bighour;
  GColor windowcolor;
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

// Persistent storage key
#define SETTINGS_KEY 1


static void handle_time_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int sec  = t->tm_sec;
  if( settings.smooth == 1 && sec % 10 == 0 ) {
    layer_mark_dirty(s_hands_layer);
    return;
  }
  layer_mark_dirty(s_hands_layer);
}

// Initialize the default settings
static void default_settings() {
  settings.color    = GColorRed;
  settings.inverted = 0;
  settings.thicker  = 0;
  settings.twentyfour = 0;
  settings.bigonly  = 0;
  settings.smooth   = 0;
  settings.bighour  = 0;
  settings.windowcolor = GColorDarkGray;
}

// Read settings from persistent storage
static void load_settings() {
  // Load the default settings
  default_settings();

  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  
}

// Save the settings to persistent storage
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
 
  // read background color setting
  Tuple *color_conf = dict_find(iter, MESSAGE_KEY_color);
  if ( color_conf ) {
    settings.color = GColorFromHEX(color_conf->value->int32);
  }
  
  // read inverted setting
  Tuple *inverted_conf = dict_find(iter, MESSAGE_KEY_inverted);
  if( inverted_conf ) { 
    settings.inverted = inverted_conf->value->int32;
  }

  // read thicker setting
  Tuple *thicker_conf = dict_find(iter, MESSAGE_KEY_thicker);
  if( thicker_conf ) { 
    settings.thicker = thicker_conf->value->int32;
  }

  // read twenty four hour setting
  Tuple *twentyfour_conf = dict_find(iter, MESSAGE_KEY_twentyfour);
  if( twentyfour_conf ) { 
    settings.twentyfour = twentyfour_conf->value->int32;
  }

  // read big only setting
  Tuple *bigonly_conf = dict_find(iter, MESSAGE_KEY_bigonly);
  if( bigonly_conf ) { 
    settings.bigonly = bigonly_conf->value->int32;
  }
  
  // read smooth transition setting
  Tuple *smooth_conf = dict_find(iter, MESSAGE_KEY_smooth);
  if( smooth_conf ) { 
    settings.smooth = smooth_conf->value->int32;
  }
  
  // read big hour setting
  Tuple *bighour_conf = dict_find(iter, MESSAGE_KEY_bighour);
  if( bighour_conf ) { 
    settings.bighour = bighour_conf->value->int32;
  }

  // read background color setting
  Tuple *windowcolor_conf = dict_find(iter, MESSAGE_KEY_windowcolor);
  if ( windowcolor_conf ) {
    settings.windowcolor = GColorFromHEX(windowcolor_conf->value->int32);
  }
  
  save_settings();
  
  if( settings.inverted == 0 ) {
    window_set_background_color(s_window, GColorBlack);
  }
  else {
    window_set_background_color(s_window, GColorWhite);
  }

  tick_timer_service_unsubscribe();
  if( settings.smooth == 0 ) {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_time_tick );
  }
  else {
    tick_timer_service_subscribe(SECOND_UNIT, handle_time_tick );
  }
     
  layer_mark_dirty( s_dot_layer );
  layer_mark_dirty( s_hands_layer );

}

static void hands_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int hour = t->tm_hour;
  int min  = t->tm_min;
  int sec  = t->tm_sec;
    
  if( settings.twentyfour == 0 ) {
    if( hour > 12 ) {
      hour = hour - 12;
    }
    if( hour == 0 ) {
      hour = 12;
    }
  }   
  
  int angle = 0;
 
  GPoint inner, outer;
  int inner_pos, outer_pos;
  GRect bounds  = layer_get_bounds(s_hands_layer);
  GPoint center = GPoint( bounds.size.w/2, bounds.size.h/2);
    
  graphics_context_set_antialiased(ctx, true);
  
  // draw hour window
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, settings.windowcolor);
    graphics_context_set_stroke_color(ctx, settings.windowcolor);
  #else
    if( settings.inverted == 0 ) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_context_set_stroke_color(ctx, GColorWhite);
    }
    else {
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_context_set_stroke_color(ctx, GColorBlack);
    }
  #endif

  //graphics_fill_circle(ctx, GPoint(40,center.y), 15);
  #ifdef PBL_COLOR
    graphics_fill_rect(ctx, GRect(22, center.y-15, 36, 30), 8, GCornersAll );
  #else
    graphics_draw_round_rect(ctx, GRect(22, center.y-15, 36, 30), 8 );
  #endif

  char buf[10];
  GFont font;
  GRect text;
  if( settings.inverted == 0 ) {
    graphics_context_set_text_color(ctx, GColorWhite );
  }
  else {
    graphics_context_set_text_color(ctx, GColorBlack );
  }
  
  if( settings.bighour == 0 ) {
    snprintf( buf, sizeof(buf), "%d", hour );
    font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    text = GRect(25, center.y-17, 30, 24);
  }
  else {
    snprintf( buf, sizeof(buf), "%d", hour );
    font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    text = GRect(25, center.y-19, 30, 24);
  }  
  
  if( min >= 30 && settings.bigonly == 0 ) {
    snprintf( buf, sizeof(buf), "%d:30", hour );
    font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    text = GRect(25, center.y-9, 30, 14);
  }
  graphics_draw_text(ctx, buf, font, text, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  
  
  // minute hand
  angle = DEG_TO_TRIGANGLE(min*6+sec/10);
  inner_pos = bounds.size.w*-30/200;
  outer_pos = bounds.size.w*90/200;
  if( settings.thicker == 0 ) {
    graphics_context_set_stroke_width(ctx, 3);
  }
  else {
    graphics_context_set_stroke_width(ctx, 5);
  }
  
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, settings.color );
    graphics_context_set_stroke_color(ctx, settings.color );
  #else
    if( settings.inverted == 0 ) {
      graphics_context_set_fill_color(ctx, GColorWhite );
      graphics_context_set_stroke_color(ctx, GColorWhite );
    }
    else {
      graphics_context_set_fill_color(ctx, GColorBlack );
      graphics_context_set_stroke_color(ctx, GColorBlack );
    }
  #endif

  inner.y = (int16_t)(-cos_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.y;
  inner.x = (int16_t)(sin_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.x;
  outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.y;
  outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.x;
  // draw minute hand
  graphics_draw_line(ctx, inner, outer);
  // draw circle in the middle
  graphics_fill_circle(ctx, center, 6);
  // draw circle as counter weight
  graphics_fill_circle(ctx, inner, 3);  
    
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_fill_circle(ctx, center, 2);
  
}

static void dot_update_proc(Layer *layer, GContext *ctx) {
  
  GRect bounds  = layer_get_bounds(s_dot_layer);
  GPoint center = GPoint( bounds.size.w/2, bounds.size.h/2);
  
  int inner_pos, outer_pos;
  int angle = 0;
  GPoint inner, outer;
  
  // minute marks
  inner_pos = bounds.size.w*80/200;
  outer_pos = bounds.size.w*100/200;
  
  if( settings.inverted == 0 ) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
  else {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
  }
  
  if( settings.thicker == 0 ) {
    graphics_context_set_stroke_width(ctx, 1);
  }
  else {
    graphics_context_set_stroke_width(ctx, 3);
  }
  for (int i=0; i<=60; i+=1) {
	  angle = DEG_TO_TRIGANGLE(i*6);
  	inner.y = (int16_t)(-cos_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.y;
	  inner.x = (int16_t)(sin_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.x;
	  outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.y;
	  outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.x;
	  // Draw tick mark
     graphics_draw_line(ctx, inner, outer);
  }  
  // cut to real value
  inner_pos = bounds.size.w*90/200;
  if( settings.inverted == 0 ) {
    graphics_context_set_fill_color( ctx, GColorBlack );
  }
  else {
    graphics_context_set_fill_color( ctx, GColorWhite );
  }
  graphics_fill_circle( ctx, center, inner_pos );
  
  // 5 minute marks
  inner_pos = bounds.size.w*80/200;
  outer_pos = bounds.size.w*100/200;
  if( settings.inverted == 0 ) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
  else {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);    
  }
  for (int i=0; i<=60; i+=5) {
	  angle = DEG_TO_TRIGANGLE(i*6);
   	inner.y = (int16_t)(-cos_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.y;
	  inner.x = (int16_t)(sin_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.x;
	  outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.y;
	  outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.x;
    // Draw tick mark
    graphics_draw_line(ctx, inner, outer);
  }  
  
  // quarter minute marks
  inner_pos = bounds.size.w*80/200;
  outer_pos = bounds.size.w*100/200;
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, settings.color );
    graphics_context_set_stroke_color(ctx, settings.color );
  #else
    if( settings.inverted == 0 ) {
      graphics_context_set_fill_color(ctx, GColorWhite );
      graphics_context_set_stroke_color(ctx, GColorWhite );
    }
    else {
      graphics_context_set_fill_color(ctx, GColorBlack );
      graphics_context_set_stroke_color(ctx, GColorBlack );
    }
  #endif
  for (int i=0; i<=60; i+=15) {
	  angle = DEG_TO_TRIGANGLE(i*6);
   	inner.y = (int16_t)(-cos_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.y;
	  inner.x = (int16_t)(sin_lookup(angle) * (int32_t)inner_pos / TRIG_MAX_RATIO) + center.x;
	  outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.y;
	  outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + center.x;
	  // Draw tick mark
    graphics_draw_line(ctx, inner, outer);
  }  
  
  // set name of watchface
  if( settings.inverted == 0 ) {
    graphics_context_set_text_color(ctx, GColorWhite );
  }
  else {
    graphics_context_set_text_color(ctx, GColorBlack );
  }
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  GRect text = GRect(center.x, center.y-9, bounds.size.w/2, 14);
  graphics_draw_text(ctx, "GANSKE", font, text, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
}

  
static void app_focus_changing(bool focusing) {
	if (focusing) {
	   layer_set_hidden(window_get_root_layer(s_window), true); 
	}
}

static void app_focus_changed(bool focused) {
  if (focused) {
    layer_set_hidden(window_get_root_layer(s_window), false); 
    layer_mark_dirty(window_get_root_layer(s_window));
  }
}

static void window_load(Window *window) {
  load_settings();
  
  if( settings.inverted == 0 ) {
    window_set_background_color(s_window, GColorBlack);
  }
  else {
    window_set_background_color(s_window, GColorWhite);
  }

  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));

  s_dot_layer = layer_create(bounds);
  layer_set_update_proc(s_dot_layer, dot_update_proc);
  layer_add_child(window_get_root_layer(s_window), s_dot_layer);  
  
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_get_root_layer(s_window), s_hands_layer);
  
  app_focus_service_subscribe_handlers((AppFocusHandlers){
	  .did_focus = app_focus_changed,
	  .will_focus = app_focus_changing
	});
  
  if( settings.smooth == 0 ) {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_time_tick );
  }
  else {
    tick_timer_service_subscribe(SECOND_UNIT, handle_time_tick );
  }
}

static void window_unload(Window *window) {
}

static void init() {
  s_window = window_create();
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  //Register AppMessage events
  app_message_register_inbox_received(in_received_handler);
  //Largest possible input and output buffer sizes  
  app_message_open(128,128);
  
  window_stack_push(s_window, true);
}

static void deinit() {
  save_settings();
  app_focus_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}