#include <pebble.h>

static Window *s_main_window; // Main window
static Layer *s_window_layer, *s_foreground_layer; // Window layer to add other layers to and the foreground layer
static char s_time_text[6] = "00:00", s_battery_text[5] = "100%", s_steps_text[6], s_temperature_buffer[8], s_conditions_buffer[32], s_weather_text[32]; // Text to put time and battery state into
static GFont s_leco_font;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_temperature);
	Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_conditions);
	
	if (temp_tuple && conditions_tuple) {
		snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%dC", (int)temp_tuple->value->int32);
		snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "%s", conditions_tuple->value->cstring);
		snprintf(s_weather_text, sizeof(s_weather_text), "%s:%s", s_temperature_buffer, s_conditions_buffer);
		layer_mark_dirty(s_foreground_layer);
	}
}

// Update procedure for foreground layer
static void foreground_update_proc(Layer *s_foreground_layer, GContext *ctx) {
	// Set bounds of window
	GRect bounds = layer_get_bounds(s_window_layer);
	
	// Set colour to black
	graphics_context_set_text_color(ctx, GColorBlack);
	
	// Draw time text
	GSize time_text_bounds = graphics_text_layout_get_content_size(s_time_text, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_time_text, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), GRect((bounds.size.w - time_text_bounds.w) / 2, bounds.size.h / 2 - time_text_bounds.h / 2, time_text_bounds.w, time_text_bounds.h),
										 GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	
	// Draw battery text
	GSize battery_text_bounds = graphics_text_layout_get_content_size(s_battery_text, s_leco_font, GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_battery_text, s_leco_font, GRect((bounds.size.w - battery_text_bounds.w) / 2, bounds.size.h / 2 - time_text_bounds.h / 2 - battery_text_bounds.h, battery_text_bounds.w, battery_text_bounds.h),
										 GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	
	// Draw health text
	GSize step_text_bounds = graphics_text_layout_get_content_size(s_steps_text, s_leco_font, GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_steps_text, s_leco_font, GRect((bounds.size.w - step_text_bounds.w) / 2, bounds.size.h / 2 + time_text_bounds.h / 2 + step_text_bounds.h, step_text_bounds.w, step_text_bounds.h),
										 GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	
	// Draw weather text
	GSize weather_text_bounds = graphics_text_layout_get_content_size(s_weather_text, s_leco_font, GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_weather_text, s_leco_font, GRect((bounds.size.w - weather_text_bounds.w) / 2, weather_text_bounds.h / 2, weather_text_bounds.w, weather_text_bounds.h),
										GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void update_time() {
	// Get current time and put into string
	time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  strftime(s_time_text, sizeof(s_time_text), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
}


// Handler for when minute changes - update the UI.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	// Redraw the screen
	layer_mark_dirty(s_foreground_layer);
}

// Handles changes upon event for battery
static void battery_handler() {
	// Get current battery level and put into string
	BatteryChargeState state = battery_state_service_peek();
  int s_battery_level = state.charge_percent;
	snprintf(s_battery_text, sizeof(s_battery_text), "%d%%", s_battery_level);
	// Redraw the screen
	layer_mark_dirty(s_foreground_layer);
}

#if defined(PBL_HEALTH)
static void update_step_count() {
	HealthValue value = health_service_sum_today(HealthMetricStepCount);
	if ((int)value >= 1000) {
		int thousands = value / 1000;
		int hundreds = value % 1000 / 100;
		snprintf(s_steps_text, sizeof(s_steps_text), "%d.%dk", thousands, hundreds);
	} else {
		snprintf(s_steps_text, sizeof(s_steps_text), "%d", (int)value);
	}
}

// Handles changes upon event in health
static void health_handler(HealthEventType event, void *context) {
	update_step_count();
}
#endif

static void initialize_ui() {
	GRect bounds = layer_get_bounds(s_window_layer);
	
	// Create foreground layer, set update procedures, and add to window.
	s_foreground_layer = layer_create(bounds);
	layer_set_update_proc(s_foreground_layer, foreground_update_proc);
	layer_add_child(window_get_root_layer(s_main_window), s_foreground_layer);
}

static void main_window_load(Window *window) {
	s_window_layer = window_get_root_layer(window);
	initialize_ui();
	update_time();
	battery_handler();
	#if defined(PBL_HEALTH)
	update_step_count();
	#endif
	layer_mark_dirty(s_foreground_layer);
}

// Destroy layers upon unloading
static void main_window_unload(Window *window) {
	layer_destroy(s_foreground_layer);
}

static void init() {
	app_message_register_inbox_received(inbox_received_callback);
	app_message_open(128, 128);
	
	// Create window and set mmethods
	s_main_window = window_create();
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	// Load custom font
	s_leco_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LECO_20));
	
	window_stack_push(s_main_window, true);
	
	// Subscribe to the clock
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Subscribe to the battery level
	battery_state_service_subscribe(battery_handler);
	
	// Subscribe to the health service if Pebble Health is available
	#if defined(PBL_HEALTH)
		health_service_events_subscribe(health_handler, NULL);
	#endif
}

// Destroy main window upon leaving
static void deinit() {
	fonts_unload_custom_font(s_leco_font);
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}