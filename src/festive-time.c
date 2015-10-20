#include <pebble.h>
#include <time.h>
#include <stdlib.h>

#define KEY_TEMPERATURE 0

#define KEY_TWENTY_FOUR_HOUR_FORMAT 1
#define KEY_BATTERY_ON_OFF 2
#define KEY_TEMP_TYPE 3
#define KEY_BIRTHDAY_LIST 4
#define KEY_INVERT_COLOR 5

#define KEY_BDAY_LIST_SIZE 20

typedef struct
{
	char date[5];
	char name[10];
} Birthday;
    
enum Colors{
	BLACK,
	WHITE
};

static Window *window;
static TextLayer *s_time_layer, *s_date_layer, *s_weekday_layer, *s_weather_layer;

static bool twenty_four_hour_format = false;
static bool battery_on_off = false;

static char* temperature_format = "Fahrenheit";
static int initTemp = 5000;

static char birthday_list[(sizeof(Birthday)+1)*10];
static int birthday_list_size = 0;

static int background_color = BLACK;
static int foreground_color = WHITE;
static bool is_inverted = false;

// Battery
static Layer *s_battery_layer;
static Layer *s_divider_layer;
static int s_battery_level;

// Bluetooth
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

// Background
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// Birthday
static Birthday birthdays[10];

/** Update bluetooth logic **/
static void bluetooth_callback(bool connected){
    // Show icon if disconnected
    layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
    
    if(!connected){
        // Issue a vibrating alert
        vibes_double_pulse();
    }
}

/** Render battery indicator **/
static void battery_update_proc(Layer *layer, GContext *ctx){
    GRect bounds = layer_get_bounds(layer);
    
    // Find the width of the bar
    int width = (int)(float)(((float)s_battery_level / 100.0F) * 123.0F);
    
    // Draw the background
    graphics_context_set_fill_color(ctx, background_color ? GColorWhite : GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Draw the bar	
	if(battery_on_off){
		graphics_context_set_fill_color(ctx, foreground_color ? GColorWhite : GColorBlack);
		graphics_fill_rect(ctx, GRect((123 - width) / 2, 0, width, 3), 0, GCornerNone);
    }
    // Draw the static bars
    graphics_context_set_fill_color(ctx, foreground_color ? GColorWhite : GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 1, 123, 1), 0, GCornerNone);
}

/** Render divider **/
static void divider_update_proc(Layer *layer, GContext *ctx){
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, foreground_color ? GColorWhite : GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

/** Update battery logic **/
static void battery_callback(BatteryChargeState state){
    // Record the new battery level
    s_battery_level = state.charge_percent;
    
    // Update meter
    layer_mark_dirty(s_battery_layer);
}

/** Get resource for normal weekdays **/
static uint16_t normalWeekday(int weekday){
	switch(weekday){
		case 0: return RESOURCE_ID_IMAGE_SUNDAY;
        case 1: return RESOURCE_ID_IMAGE_MONDAY;
        case 2: return RESOURCE_ID_IMAGE_TUESDAY;
		case 3: return RESOURCE_ID_IMAGE_CAMEL;
        case 4: return RESOURCE_ID_IMAGE_THURSDAY;
		case 5: return RESOURCE_ID_IMAGE_FRIDAY;
        case 6: return RESOURCE_ID_IMAGE_SATURDAY;
		default: break;		
	}
	return 0;
}

static int countChar(char* string, char delim){
	int charCount = 0;
	for(int i = 0; i < (int)strlen(string); i++){
		if(string[i] == delim)
			charCount++;
	}
	return charCount + 1;
}

/** Get background resource for special days **/
static uint16_t get_background_resource(int month, int day, int weekday){
	text_layer_set_text(s_weekday_layer, NULL);
	// Check for birthdays
	for(int i = 0; i < 10; i++){
		if(birthdays[i].name == NULL)
			continue;

	    int bday_month_tens = ((birthdays[i].date)[0] - '0')*10; 
	    int bday_month_ones = ((birthdays[i].date)[1] - '0');
	    int bday_month = bday_month_tens + bday_month_ones;
	    
	    int bday_day_tens = ((birthdays[i].date)[3] - '0')*10;
	    int bday_day_ones = ((birthdays[i].date)[4] - '0');
	    int bday_day = bday_day_tens + bday_day_ones;

	    if(bday_month == month && bday_day == day){
	        char* bdaystr = "";
			strcpy(bdaystr, (char*)birthdays[i].name);
	        strcat(bdaystr, "'s Birthday!");
	        text_layer_set_text(s_weekday_layer, bdaystr);
	        return RESOURCE_ID_IMAGE_BIRTHDAY;
	    }
	}

    switch(month){
		case 1:
			switch(day){
				case 1: 
	        		text_layer_set_text(s_weekday_layer, "Happy New Year!");
					return RESOURCE_ID_IMAGE_NEW_YEARS;
			}
		case 2:
			switch(day){
				case 14: 
	        		text_layer_set_text(s_weekday_layer, "Valentine's Day!");
					return RESOURCE_ID_IMAGE_VALENTINE;
			}
		case 3:
			switch(day){
				case 17: 
	        		text_layer_set_text(s_weekday_layer, "St. Patrick's Day!");
					return RESOURCE_ID_IMAGE_ST_PATRICK;
				case 20:
				case 21: 
	        		text_layer_set_text(s_weekday_layer, "Happy Spring!");
					return RESOURCE_ID_IMAGE_SPRING;
			}
        case 4:
            switch(day){
				case 1: 
	        		text_layer_set_text(s_weekday_layer, "April Fools!");
					return RESOURCE_ID_IMAGE_APRIL_FOOLS;
				default: return RESOURCE_ID_IMAGE_RABBIT;
			}
		case 5:
			switch(day){
				case 5: 
	        		text_layer_set_text(s_weekday_layer, "Happy Cinco de Mayo!");
					return RESOURCE_ID_IMAGE_CINCO_DE_MAYO;
			}
		case 6:
			switch(day){
				case 22:
				case 23: 
	        		text_layer_set_text(s_weekday_layer, "Happy Summer!");
					return RESOURCE_ID_IMAGE_SUMMER;
			}
		case 7:
			switch(day){
				case 4: 
	        		text_layer_set_text(s_weekday_layer, "Happy 4th of July!");
					return RESOURCE_ID_IMAGE_FOURTH_OF_JULY;
			}
		case 9:
			switch(day){
				case 22:
				case 23: 
	        		text_layer_set_text(s_weekday_layer, "Happy Fall!");
					return RESOURCE_ID_IMAGE_FALL;
			}
        case 10:
            switch(day){
				case 25:
				case 26:
				case 27:
				case 28:
                case 29:
                case 30:
	        		text_layer_set_text(s_weekday_layer, NULL);
					return RESOURCE_ID_IMAGE_10_31;
                case 31: 
	        		text_layer_set_text(s_weekday_layer, "Happy Halloween!");
					return RESOURCE_ID_IMAGE_10_31;
                default: return normalWeekday(weekday);
            }
        case 11:
            return RESOURCE_ID_IMAGE_TURKEY;
        case 12:
            switch(day){
                case 21: 
                case 22: 
	        		text_layer_set_text(s_weekday_layer, "Happy Winter!");
					return RESOURCE_ID_IMAGE_WINTER;
                case 23:
				case 24:
                case 25: 
	        		text_layer_set_text(s_weekday_layer, "Merry Christmas!");
					return RESOURCE_ID_IMAGE_12_25;
                default: return normalWeekday(weekday);
            }
        default:
            return normalWeekday(weekday);
    }
    return 0;
}

/** Renders the time and date **/
static void update_time(){
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Create a long-lived buffer
    static char buffer[] = "00:00";
    static char date_buffer[] = "DDD, MMM DD";

    // Write the current hours and minutes into the buffer
    if(twenty_four_hour_format){
        // Use 24 hour format
        strftime(buffer, sizeof("00:00"), "%k:%M", tick_time);
    } else {
        // Use 12 hour format
        strftime(buffer, sizeof("00:00"), "%l:%M", tick_time);
    }

    strftime(date_buffer, sizeof("DDD, MMM DD"), "%a, %b %e", tick_time);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Time -> %s", buffer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Date -> %s", date_buffer);

    text_layer_set_text(s_time_layer, buffer);
    text_layer_set_text(s_date_layer, date_buffer);
}

/** Renders the background image **/
static void update_image(){

    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Create a long-lived buffer
    static char date_buffer[] = "DDD, MMM DD";
    strftime(date_buffer, sizeof("DDD, MMM DD"), "%a, %b %e", tick_time);
    
    // Update background based on day
    char month[3], day[3], weekday[2];
    strftime(month, sizeof(month), "%m", tick_time);
    strftime(day, sizeof(day), "%d", tick_time);
    strftime(weekday, sizeof(weekday), "%w", tick_time);

    uint16_t RESOURCE_ID = get_background_resource(atoi(month), atoi(day), atoi(weekday));
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID);
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    if(s_background_bitmap == NULL){
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Image Missing: [%i]", RESOURCE_ID);
    }

	if(text_layer_get_text(s_weekday_layer) == NULL)
		text_layer_set_background_color(s_weekday_layer, GColorClear);
	else
		text_layer_set_background_color(s_weekday_layer, background_color ? GColorWhite : GColorBlack);
}

static void parse_birthday_list(char* list){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "The birthday list is: %s", list);
	char* temp = list;
	if(temp != NULL){
		int listSize = countChar(list, ',');
		if(listSize%2 != 0) 
			return;

		int index = 0;
		for(int i = 0; i < listSize && i < 16*2; i+=2){
			char* src = temp;		
		
			// Name
			int nameSize = strcspn(src, ",");
			char name[10];		
			memset(name, '\0', sizeof(name));
			strncpy(name, src, nameSize);
	
			// Date
			int dateSize = strcspn(src + nameSize + 1, ",");
			char date[5];
			memset(date, '\0', sizeof(date));
			strncpy(date, src + nameSize + 1, dateSize);

			strncpy(birthdays[index].name, name, sizeof(birthdays[index].name));

			strncpy(birthdays[index].date, date, sizeof(birthdays[index].date));

			strncpy(temp, temp + nameSize + dateSize + 2, strlen(temp));

			index++;
		}
	}
	update_image();
}

static void update_temperature(char* temp_format){
    static char temperature_buffer[8];
	int temperature = 0;

	printf(temp_format);

	if(temp_format[0] == 'F'){
		temperature = initTemp;
		snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", temperature);
	}else if(temp_format[0] == 'C'){
		temperature = (initTemp - 32) * 5 / 9;
		snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", temperature);
	}else{
		temperature = (int)((double)initTemp + 459.67) * 5 / 9;
		snprintf(temperature_buffer, sizeof(temperature_buffer), "%d K", temperature);
	}

	if(initTemp == 5000)
    	text_layer_set_text(s_weather_layer, "...");
	else	
	    text_layer_set_text(s_weather_layer, temperature_buffer);
    layer_mark_dirty(text_layer_get_layer(s_weather_layer));
}

/** Updates time logic **/
static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
    if( (units_changed & MINUTE_UNIT) != 0){
        update_time();
        
        // Get weather update every 30 minutes
        if(tick_time->tm_min % 30 == 0) {
            // Begin dictionary
            DictionaryIterator *iter;
            app_message_outbox_begin(&iter);

            // Add a key-value pair
            dict_write_uint8(iter, 0, 0);

            // Send the message!
            app_message_outbox_send();
			update_temperature(temperature_format);
        }
    }
    
    if( (units_changed & DAY_UNIT) != 0){
        update_image();
        layer_mark_dirty(bitmap_layer_get_layer(s_background_layer));
    }        
}

/** Create elements in window **/
static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);

	// check for color inversion
	if(persist_exists(KEY_INVERT_COLOR)){
		is_inverted = persist_read_bool(KEY_INVERT_COLOR);
		if(is_inverted){
			background_color = WHITE;
			foreground_color = BLACK;
		}else{
			background_color = BLACK;
			foreground_color = WHITE;
		}
	}

    window_set_background_color(window, background_color ? GColorWhite : GColorBlack);

    // Create GBitmap, then set to created BitmapLayer
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAMEL);
    s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 88));
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	bitmap_layer_set_compositing_mode(s_background_layer, GCompOpAssign);
	if(is_inverted)
		bitmap_layer_set_compositing_mode(s_background_layer, GCompOpAssignInverted);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

    // Create time TextLayer
    s_time_layer = text_layer_create(GRect(0, 85, 144, 50));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, foreground_color ? GColorWhite : GColorBlack);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

    // Create date TextLayer
    s_date_layer = text_layer_create(GRect(0, 135, 90, 50));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, foreground_color ? GColorWhite : GColorBlack);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));

    // Create weekday TextLayer
    s_weekday_layer = text_layer_create(GRect(2, 52, 140, 20));
    text_layer_set_text_alignment(s_weekday_layer, GTextAlignmentCenter);
    text_layer_set_background_color(s_weekday_layer, GColorBlack);
    text_layer_set_text_color(s_weekday_layer, foreground_color ? GColorWhite : GColorBlack);
    text_layer_set_font(s_weekday_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    
    // Create battery layer
    s_battery_layer = layer_create(GRect(10, 135, 123, 3));
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    
    // Create divider layer
    s_divider_layer = layer_create(GRect(90, 141, 1, 24));
    layer_set_update_proc(s_divider_layer, divider_update_proc);
    
    // Create the Bluetooth layer
    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
    s_bt_icon_layer = bitmap_layer_create(GRect(59, 12, 30, 30));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpAssign);
	if(is_inverted)
		bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpAssignInverted);	
    bluetooth_callback(bluetooth_connection_service_peek());
    
    // Create temperature layer
    s_weather_layer = text_layer_create(GRect(90, 135, 59, 50));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, foreground_color ? GColorWhite : GColorBlack);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_text(s_weather_layer, "...");

    // Add layers to window
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_weekday_layer));
    layer_add_child(window_layer, s_battery_layer);
    layer_add_child(window_layer, s_divider_layer);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

	/* Check persist items */
	if(persist_exists(KEY_TWENTY_FOUR_HOUR_FORMAT)){
		twenty_four_hour_format = persist_read_bool(KEY_TWENTY_FOUR_HOUR_FORMAT);
		update_time();
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Time: [%s]", (twenty_four_hour_format) ? "24 hours" : "12 hours");	
	}

	if(persist_exists(KEY_BATTERY_ON_OFF)){
		battery_on_off = persist_read_bool(KEY_BATTERY_ON_OFF);
		layer_mark_dirty(s_battery_layer);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "Battery: [%s]", (battery_on_off) ? "On" : "Off");	
	}

	if(persist_exists(KEY_TEMP_TYPE)){
		persist_read_string(KEY_TEMP_TYPE, temperature_format, sizeof("Fahrenheit"));
		update_temperature(temperature_format);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Temp style: [%s]", temperature_format);
	}

	if(persist_exists(KEY_BDAY_LIST_SIZE)){
		birthday_list_size = persist_read_int(KEY_BDAY_LIST_SIZE);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Bday List Size: [%i]", birthday_list_size);
	}

	if(persist_exists(KEY_BIRTHDAY_LIST)){
		persist_read_string(KEY_BIRTHDAY_LIST, birthday_list, birthday_list_size);
		parse_birthday_list(birthday_list);
	}
}

/** Free up the memory on delete **/
static void window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_weekday_layer);
    gbitmap_destroy(s_background_bitmap);
    bitmap_layer_destroy(s_background_layer);
    layer_destroy(s_battery_layer);
    gbitmap_destroy(s_bt_icon_bitmap);
    bitmap_layer_destroy(s_bt_icon_layer);
    text_layer_destroy(s_weather_layer);
    layer_destroy(s_divider_layer);
}

/** JS AppMessages **/
static void inbox_received_callback(DictionaryIterator *iterator, void *context){

	// Read first item
    Tuple *t = dict_read_first(iterator);
    
    // For all items
    while(t != NULL) {
        switch(t->key){
            case KEY_TEMPERATURE:
                initTemp = (int)t->value->int32;
				update_temperature(temperature_format);
                break;			
			case KEY_TWENTY_FOUR_HOUR_FORMAT:
				twenty_four_hour_format = (bool)t->value->int8;
				persist_write_bool(KEY_TWENTY_FOUR_HOUR_FORMAT, twenty_four_hour_format);
				update_time();
				break;
			case KEY_BATTERY_ON_OFF:
				battery_on_off = (bool)t->value->int8;
				persist_write_bool(KEY_BATTERY_ON_OFF, battery_on_off);
				layer_mark_dirty(s_battery_layer);
				break;
			case KEY_TEMP_TYPE:
				strcpy(temperature_format, (char*)t->value);
				persist_write_string(KEY_TEMP_TYPE, temperature_format);
				update_temperature(temperature_format);
				break;
			case KEY_BIRTHDAY_LIST:
			{
				strcpy(birthday_list, (char*)t->value);
				persist_write_string(KEY_BIRTHDAY_LIST, birthday_list);
				persist_write_int(KEY_BDAY_LIST_SIZE, strlen((char*)t->value)+1);
				parse_birthday_list(birthday_list); 
				break;
			}
			case KEY_INVERT_COLOR:
				is_inverted = (bool)t->value->int8;
				if(is_inverted){
					background_color = WHITE;
					foreground_color = BLACK;
				}else{
					background_color = BLACK;
					foreground_color = WHITE;
				}
				persist_write_bool(KEY_INVERT_COLOR, is_inverted);
				window_unload(window);
				window_load(window);
				update_image();
				break;
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
                break;
        }
        t = dict_read_next(iterator);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true);

    // Register for time and date updates
    tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);
    update_time();
    update_image();
    
    // Register for battery level updates
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());
    
    // Register for Bluetooth connection updates
    bluetooth_connection_service_subscribe(bluetooth_callback);
    bluetooth_callback(bluetooth_connection_service_peek());

    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
	window_destroy(window);
	
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
