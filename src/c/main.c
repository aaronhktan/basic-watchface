#include <pebble.h>

static Window *s_main_window; // Main window
static Layer *s_window_layer, *s_foreground_layer; // Window layer to add other layers to and the foreground layer
static char s_time_text[6] = "00:00", s_battery_text[5] = "100%"; // Text to put time and battery state into
static GFont s_leco_font;

// Update procedure for foreground layer
static void foreground_update_proc(Layer *s_foreground_layer, GContext *ctx) {
	// Set bounds of window
	GRect bounds = layer_get_bounds(s_window_layer);
	
	// Set colour to black
	graphics_context_set_text_color(ctx, GColorBlack);
	
	// Draw time text
	GSize time_text_bounds = graphics_text_layout_get_content_size("24:00", fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_time_text, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), GRect((bounds.size.w - time_text_bounds.w) / 2, bounds.size.h / 2 - time_text_bounds.h / 2, time_text_bounds.w, time_text_bounds.h),
										 GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	
	// Draw battery text
	GSize battery_text_bounds = graphics_text_layout_get_content_size("100%", s_leco_font, GRect(0, 0, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	graphics_draw_text(ctx, s_battery_text, s_leco_font, GRect((bounds.size.w - battery_text_bounds.w) / 2, bounds.size.h / 2 - time_text_bounds.h / 2 - battery_text_bounds.h, battery_text_bounds.w, battery_text_bounds.h),
										 GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// Update the UI upon detecting a change in the minutes or change in battery percentage
static void update_ui() {
	// Get current time and put into string
	time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  strftime(s_time_text, sizeof(s_time_text), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
	
	// Get current battery level and put into string
	BatteryChargeState state = battery_state_service_peek();
  int s_battery_level = state.charge_percent;
	snprintf(s_battery_text, sizeof(s_battery_text), "%d%%", s_battery_level);
	
	// Redraw the screen
	layer_mark_dirty(s_foreground_layer);
}

// Handler for when minute changes - update the UI.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_ui();
}

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
	update_ui();
}

// Destroy layers upon unloading
static void main_window_unload(Window *window) {
	layer_destroy(s_foreground_layer);
}

static void init() {
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
	battery_state_service_subscribe(update_ui);
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