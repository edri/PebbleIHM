#include <pebble.h>

// Menu parameters.
#define NUMBER_OF_WINDOWS 4
#define NUM_MENU_SECTIONS 4
#define NUM_FIRST_MENU_ITEMS 4
#define NUM_SECOND_MENU_ITEMS 6
#define NUM_THIRD_MENU_ITEMS 5
#define NUM_FOURTH_MENU_ITEMS 1

// Android Communication
#define REQUEST_LOCATION                0
#define REQUEST_FIX_LOCATION            1
#define REQUEST_START_THREADED_LOCATION 2
#define REQUEST_STOP_THREADED_LOCATION  3
#define REQUEST_ELEVATION               4
#define REQUEST_WEATHER_STATUS          5
#define REQUEST_WEATHER_TEMPERATURE     6
#define REQUEST_WEATHER_PRESSURE        7
#define REQUEST_WEATHER_HUMIDITY        8
#define REQUEST_WEATHER_WIND            9
#define REQUEST_WEATHER_SUNRISE        10
#define REQUEST_WEATHER_SUNSET         11
#define REQUEST_TRANSPORT              12
#define SHOW_UP_TIME                   13
#define SHOW_ACTIVE_TIME               14
#define SHOW_BATTERY_STATE             15

#define NUMBER_OF_ITEMS                16


// Pebble KEY
#define PEBBLE_KEY_VALUE        1
// Location API
#define KEY_LATITUDE        100
#define KEY_LONGITUDE       101
#define KEY_DISTANCE        102
#define KEY_DIRECTION       103
// Elevation API
#define KEY_ALTITUDE        200
// Weather API
#define KEY_STATUS          300
#define KEY_DESCRIPTION     301
#define KEY_TEMPERATURE     302
#define KEY_PRESSURE        303
#define KEY_HUMIDITY        304
#define KEY_WIND_SPEED      305
#define KEY_WIND_DIRECTION  306
#define KEY_SUNRISE         307
#define KEY_SUNSET          308
// Transport API
#define KEY_DEPARTURE       400
#define KEY_DEPARTURE_TIME  401
#define KEY_ARRIVAL         402
#define KEY_ARRIVAL_TIME    403


#define MAX_TEXT_SIZE       128
#define NUM_ACCEL_SAMPLES   10
#define GRAVITY             10000 // (1g)² = 10000
#define ACCEL_THRESHOLD     8000  // (1g)² = 10000

// Menu items
#define ITEM_LOCATION        0
#define ITEM_DISTANCE        1
#define ITEM_HEADING         2
#define ITEM_ELEVATION       3
#define ITEM_STATUS          4
#define ITEM_TEMPERATURE     5
#define ITEM_PRESSURE        6
#define ITEM_HUMIDITY        7
#define ITEM_WIND_SPEED      8
#define ITEM_WIND_DIRECTION  9
#define ITEM_SUNRISE         10
#define ITEM_SUNSET          11
#define ITEM_TRAIN           12
#define ITEM_UPTIME          13
#define ITEM_TIME            14
#define ITEM_BATTERY         15

static Window  *window_1, *window_2, *window_3, *window_4,
               *menu_window;
TextLayer *layer_window_number,
          *separation_layer_1, *separation_layer_2,
          *layer_1_window_1, *layer_2_window_1, 
          *layer_1_window_2, *layer_2_window_2,
          *layer_1_window_3, *layer_2_window_3, 
          *layer_1_window_4, *layer_2_window_4;
// Menu.
static SimpleMenuLayer *s_simple_menu_layer;
// Menu's sections.
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
// Each section's items.
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
static SimpleMenuItem s_second_menu_items[NUM_SECOND_MENU_ITEMS];
static SimpleMenuItem s_third_menu_items[NUM_THIRD_MENU_ITEMS];
static SimpleMenuItem s_fourth_menu_items[NUM_FOURTH_MENU_ITEMS];

// Will contain all application's windows.
Window* windows[NUMBER_OF_WINDOWS];
// Will contain current window's number.
char window_number[10] = "Window #1\0";

// Will contain all windows text layers.
TextLayer* text_layers[NUMBER_OF_WINDOWS][2];
// Will contain all layers user's choice of data.
int layers_data_choices[NUMBER_OF_WINDOWS][2];
// Indicates the current selected window's number.
unsigned int current_selected_window;
// If current selected layer is -1, no layer is selected.
int current_selected_layer;
// Will contain each windows layers texts.
char layers_texts[NUMBER_OF_WINDOWS * 2][MAX_TEXT_SIZE];

int counter = -1;
char text[MAX_TEXT_SIZE];
unsigned long int up_time = 0;      //in seconds
unsigned long int active_time = 0;  //in seconds/10


void send(int key, char *value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, key, value);
  app_message_outbox_send();
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Get time since launch
  int seconds = up_time % 60;
  int minutes = (up_time % 3600) / 60;
  int hours = up_time / 3600;

  snprintf(text, MAX_TEXT_SIZE, "Uptime:\n%dh %dm %ds", hours, minutes, seconds);

  // Display data in right windows layer.
  for (unsigned int i = 0; i < NUMBER_OF_WINDOWS; ++i)
  {
    for (unsigned j = 0; j < 2; ++j) {
      if (layers_data_choices[i][j] == ITEM_UPTIME) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
      if (layers_data_choices[i][j] == ITEM_UPTIME) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
    }
  }

  BatteryChargeState charge_state = battery_state_service_peek();

  if (charge_state.is_charging) {
    snprintf(text, MAX_TEXT_SIZE, "Battery is charging");
  }
  else {
    snprintf(text, MAX_TEXT_SIZE, "Battery is\n%d%% charged", charge_state.charge_percent);
  }

  // Display data in right windows layer.
  for (unsigned int i = 0; i < NUMBER_OF_WINDOWS; ++i)
  {
    for (unsigned j = 0; j < 2; ++j) {
      if (layers_data_choices[i][j] == ITEM_BATTERY) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
      if (layers_data_choices[i][j] == ITEM_BATTERY) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
    }
  }
  
  // Increment uptime
  up_time++;
}

static void data_handler(AccelData *data, uint32_t num_samples) {  // accel from -4000 to 4000, 1g = 1000 cm/s²
  int i, x, y, z, acc_norm_2;
  for (i = 0; i < NUM_ACCEL_SAMPLES; i++) {
                                           // Divide by 10 to avoid too high values. Now from -400 to 400
    x = data[i].x / 10;                    // accel in dm/s²
    y = data[i].y / 10;                    // accel in dm/s²
    z = data[i].z / 10;                    // accel in dm/s²
                                           // 1g = 100 dm/s²  
    acc_norm_2 = (x*x) + (y*y) + (z*z);    // (1g)² = 10000
    //APP_LOG(APP_LOG_LEVEL_INFO, "%d %d %d %d", x, y, z, acc_norm_2);
    if ( ((acc_norm_2 - GRAVITY) > ACCEL_THRESHOLD) || ((GRAVITY - acc_norm_2) > ACCEL_THRESHOLD) ) {
      active_time++;
    }
  }
    
  int active_time_s = active_time / 10;
  int seconds = active_time_s % 60;
  int minutes = (active_time_s % 3600) / 60;
  int hours = active_time_s / 3600;

  snprintf(text, MAX_TEXT_SIZE, "Active time:\n%dh %dm %ds", hours, minutes, seconds);
  
  // Display data in right windows layer.
  for (unsigned int i = 0; i < NUMBER_OF_WINDOWS; ++i)
  {
    for (unsigned j = 0; j < 2; ++j) {
      if (layers_data_choices[i][j] == ITEM_TIME) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
      if (layers_data_choices[i][j] == ITEM_TIME) {
        strncpy(layers_texts[(i * 2) + j], text, MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
    }
  }
}

void received_handler(DictionaryIterator *iter, void *context) {
  Tuple *result_tuple = dict_find(iter, PEBBLE_KEY_VALUE);
  // Will contains the const values of the corresponding menu items
  // of the received data for being able to choose the right
  // windows and the right layers in which showing the received data.
  int corresponding_menu_items[2];
  corresponding_menu_items[0] = -2;
  corresponding_menu_items[1] = -2;
  unsigned int layer_number = 0;
  // The temporary text, which will be copied in the right layouts text
  // variable.
  char temp_text[2][MAX_TEXT_SIZE];
  
  switch(result_tuple->value->int32) {
    // Location API
    case REQUEST_LOCATION:
      strcpy(temp_text[0], "lat : ");
      strcat(temp_text[0], dict_find(iter, KEY_LATITUDE)->value->cstring);
      strcat(temp_text[0], "\nlon : ");
      strcat(temp_text[0], dict_find(iter, KEY_LONGITUDE)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_LOCATION;
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(temp_text[0], "distance : ");
      strcat(temp_text[0], dict_find(iter, KEY_DISTANCE)->value->cstring);
      strcat(temp_text[1], "direction : ");
      strcat(temp_text[1], dict_find(iter, KEY_DIRECTION)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_DISTANCE;
      corresponding_menu_items[layer_number++] = ITEM_HEADING;
      break;
    // Elevation API
    case REQUEST_ELEVATION:
      strcpy(temp_text[0], "altitude : ");
      strcat(temp_text[0], dict_find(iter, KEY_ALTITUDE)->value->cstring);
      strcat(temp_text[0], "m");
      corresponding_menu_items[layer_number++] = ITEM_ELEVATION;
      break;
    // Weather API
    case REQUEST_WEATHER_STATUS:
      strcpy(temp_text[0], dict_find(iter, KEY_STATUS)->value->cstring);
      strcat(temp_text[0], "\n");
      strcat(temp_text[0], dict_find(iter, KEY_DESCRIPTION)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_STATUS;
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(temp_text[0], dict_find(iter, KEY_TEMPERATURE)->value->cstring);
      strcat(temp_text[0], "°C");
      corresponding_menu_items[layer_number++] = ITEM_TEMPERATURE;
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(temp_text[0], "pressure : ");
      strcat(temp_text[0], dict_find(iter, KEY_PRESSURE)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_PRESSURE;
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(temp_text[0], "humidity : ");
      strcat(temp_text[0], dict_find(iter, KEY_HUMIDITY)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_HUMIDITY;
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(temp_text[0], "wind speed : ");
      strcat(temp_text[0], dict_find(iter, KEY_WIND_SPEED)->value->cstring);
      strcat(temp_text[0], "km/h");
      strcat(temp_text[1], "wind direction : ");
      strcat(temp_text[1], dict_find(iter, KEY_WIND_DIRECTION)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_WIND_SPEED;
      corresponding_menu_items[layer_number++] = ITEM_WIND_DIRECTION;
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(temp_text[0], "sunrise : \n");
      strcat(temp_text[0], dict_find(iter, KEY_SUNRISE)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_SUNRISE;
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(temp_text[0], "sunset : \n");
      strcat(temp_text[0], dict_find(iter, KEY_SUNSET)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_SUNSET;
      break;
    // Transport API
    case REQUEST_TRANSPORT:
      strcpy(temp_text[0], dict_find(iter, KEY_DEPARTURE)->value->cstring);
      strcat(temp_text[0], " : ");
      strcat(temp_text[0], dict_find(iter, KEY_DEPARTURE_TIME)->value->cstring);
      strcat(temp_text[0], "\n");
      strcat(temp_text[0], dict_find(iter, KEY_ARRIVAL)->value->cstring);
      strcat(temp_text[0], " : ");
      strcat(temp_text[0], dict_find(iter, KEY_ARRIVAL_TIME)->value->cstring);
      corresponding_menu_items[layer_number++] = ITEM_TRAIN;
      break;
  }
  
  // Display data in right windows layer.
  for (unsigned int i = 0; i < NUMBER_OF_WINDOWS; ++i)
  {
    for (unsigned j = 0; j < 2; ++j) {
      if (layers_data_choices[i][j] == corresponding_menu_items[0]) {
        strncpy(layers_texts[(i * 2) + j], temp_text[0], MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
      if (layers_data_choices[i][j] == corresponding_menu_items[1]) {
        strncpy(layers_texts[(i * 2) + j], temp_text[1], MAX_TEXT_SIZE);
        text_layer_set_text(text_layers[i][j], layers_texts[(i * 2) + j]);
      }
    }
  }
}

// Update window's number layer's text so it indicates the current window's
// number as the format "Window #[WINDOW_NUMBER]".
// Also unselect ALL windows layers.
void set_window_number_layer_text() {
  char cWindowNumber[2];
  snprintf(cWindowNumber, 2, "%d", (current_selected_window + 1));
  strncpy(window_number + 8, cWindowNumber, 1);
  
  // Unselects ALL windows layers.
  for (unsigned int i = 0; i < NUMBER_OF_WINDOWS; ++i) {
    text_layer_set_text_color(text_layers[i][0], GColorBlack);
    text_layer_set_background_color(text_layers[i][0], GColorWhite);
    text_layer_set_text_color(text_layers[i][1], GColorBlack);
    text_layer_set_background_color(text_layers[i][1], GColorWhite);
  }
}

void click_config_provider(void *context);
static void window_load(Window *window);

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Switch windows if the current selected one is not the topmost one.
  if (current_selected_window < (NUMBER_OF_WINDOWS - 1)) {
    window_stack_push(windows[++current_selected_window], true);
  }  
  // If the selected window is the topmost one, pops all windows to show the first one.
  else {
    for (unsigned int i = NUMBER_OF_WINDOWS - 1; i > 0; --i) {
      window_stack_pop(true);
      --current_selected_window;
    }
  }
  
  // No layer is selected yet.
  current_selected_layer = -1;
  set_window_number_layer_text();
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Removes the top topmost window of the navigation stack if the selected one is not the bottommost one.
  if (current_selected_window) {
    window_stack_pop(true);
    --current_selected_window;
  }
  // If the selected window is the bottommost one, stacks all windows to show the last one.
  else {
    for (unsigned int i = 1; i < NUMBER_OF_WINDOWS; ++i) {
      window_stack_push(windows[++current_selected_window], true);
    }
  }
  
  // No layer is selected yet.
  current_selected_layer = -1;
  set_window_number_layer_text();
}

void select_click_handler (ClickRecognizerRef recognizer, void *context) {
  // Unselects current window's layer if one of them is selected.
  if (current_selected_layer != -1) {    
    text_layer_set_text_color(text_layers[current_selected_window][current_selected_layer], GColorBlack);
    text_layer_set_background_color(text_layers[current_selected_window][current_selected_layer], GColorWhite);
  }  
  
  // Selects the next window's layer.
  current_selected_layer = (current_selected_layer + 1) % 2;
  text_layer_set_text_color(text_layers[current_selected_window][current_selected_layer], GColorWhite);
  text_layer_set_background_color(text_layers[current_selected_window][current_selected_layer], GColorBlack);
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (current_selected_layer != -1) {
    window_stack_push(menu_window, true);
  }
}

void back_click_handler (ClickRecognizerRef recognizer, void *context) {
  // Leaves the app if we aren't currently in the menu window.
  if (!window_stack_contains_window(menu_window)) {
    window_stack_pop_all(true);
  }
}

void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {}

void click_config_provider(void *context) {
  // Configures buttons actions on single clicks.
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  
  // Config a long click on the SELECT button.
  window_long_click_subscribe(BUTTON_ID_SELECT, 600, select_long_click_handler, select_long_click_release_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Initilaizes top-layout of the current window.
  text_layers[current_selected_window][0] = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h / 2 - 12)); // Change if you use PEBBLE_SDK 3
  text_layer_set_text(text_layers[current_selected_window][0], "No selected data.\nPlease longpress the \"SELECT\" button.");
  text_layer_set_text_alignment(text_layers[current_selected_window][0], GTextAlignmentCenter);
  text_layer_set_text_color(text_layers[current_selected_window][0], GColorBlack);
  text_layer_set_background_color(text_layers[current_selected_window][0], GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(text_layers[current_selected_window][0]));
  // Initializes bottom-layout of the current window.
  text_layers[current_selected_window][1] = text_layer_create(GRect(0, bounds.size.h / 2 - 9, bounds.size.w, bounds.size.h / 2 - 12)); // Change if you use PEBBLE_SDK 3
  text_layer_set_text(text_layers[current_selected_window][1], "No selected data.\nPlease longpress the \"SELECT\" button.");
  text_layer_set_text_alignment(text_layers[current_selected_window][1], GTextAlignmentCenter);
  text_layer_set_text_color(text_layers[current_selected_window][1], GColorBlack);
  text_layer_set_background_color(text_layers[current_selected_window][1], GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(text_layers[current_selected_window][1]));
  
  // Adds separations layers, which separate other layers with a black line.
  separation_layer_1 = text_layer_create(GRect(0, bounds.size.h / 2 - 11, bounds.size.w, 1)); // Change if you use PEBBLE_SDK 3
  text_layer_set_background_color(separation_layer_1, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(separation_layer_1));
  
  separation_layer_2 = text_layer_create(GRect(0, bounds.size.h - 21, bounds.size.w, 2)); // Change if you use PEBBLE_SDK 3
  text_layer_set_background_color(separation_layer_2, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(separation_layer_2));
  
  // Initialize "Window number" layer which will be on the bottom of each window to indicate
  // its number.
  layer_window_number = text_layer_create(GRect(0, bounds.size.h - 19, bounds.size.w, 19)); // Change if you use PEBBLE_SDK 3
  text_layer_set_text(layer_window_number, window_number);
  text_layer_set_text_alignment(layer_window_number, GTextAlignmentCenter);
  text_layer_set_text_color(layer_window_number, GColorBlack);
  text_layer_set_background_color(layer_window_number, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(layer_window_number));
}

static void window_1_unload(Window *window) {
  text_layer_destroy(text_layers[0][0]);
  text_layer_destroy(text_layers[0][1]);
}

static void window_2_unload(Window *window) {
  text_layer_destroy(text_layers[1][0]);
  text_layer_destroy(text_layers[1][1]);
}

static void window_3_unload(Window *window) {
  text_layer_destroy(text_layers[2][0]);
  text_layer_destroy(text_layers[2][1]);
}

static void window_4_unload(Window *window) {
  text_layer_destroy(text_layers[3][0]);
  text_layer_destroy(text_layers[3][1]);
}

static void menu_section_1_select_callback(int index, void *ctx) {  
  switch (index) {
    case 0:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nLOCATION\nsent");
      send(REQUEST_LOCATION, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_LOCATION;
      break;
    case 1:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nFIXING TARGET and START THREAD NAVIGATION\nsent");
      send(REQUEST_FIX_LOCATION, "");
      send(REQUEST_START_THREADED_LOCATION, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_DISTANCE;
      break;
    case 2:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nFIXING TARGET and START THREAD NAVIGATION\nsent");
      send(REQUEST_FIX_LOCATION, "");
      send(REQUEST_START_THREADED_LOCATION, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_HEADING;
      break;
    case 3:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nELEVATION\nsent");
      send(REQUEST_ELEVATION, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_ELEVATION;
      break;
  }
  
  text_layer_set_text(text_layers[current_selected_window][current_selected_layer], "\nWaiting for data reception...");
  window_stack_pop(true);
}

static void menu_section_2_select_callback(int index, void *ctx) {
  switch (index) {
    case 0:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nWEATHER_STATUS\nsent");
      send(REQUEST_WEATHER_STATUS, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_STATUS;
      break;
    case 1:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nTEMPERATURE\nsent");
      send(REQUEST_WEATHER_TEMPERATURE, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_TEMPERATURE;
      break;
    case 2:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nPRESSURE\nsent");
      send(REQUEST_WEATHER_PRESSURE, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_PRESSURE;
      break;
    case 3:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nHUMIDITY\nsent");
      send(REQUEST_WEATHER_HUMIDITY, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_HUMIDITY;
      break;
    case 4:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nWIND\nsent");
      send(REQUEST_WEATHER_WIND, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_WIND_SPEED;
      break;
    case 5:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nWIND\nsent");
      send(REQUEST_WEATHER_WIND, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_WIND_DIRECTION;
      break;
  }
  
  text_layer_set_text(text_layers[current_selected_window][current_selected_layer], "\nWaiting for data reception...");
  window_stack_pop(true);
}

static void menu_section_3_select_callback(int index, void *ctx) {
  switch (index) {
    case 0:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nSUNRISE\nsent");
      send(REQUEST_WEATHER_SUNRISE, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_SUNRISE;
      break;
    case 1:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nSUNSET\nsent");
      send(REQUEST_WEATHER_SUNSET, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_SUNSET;
      break;
    case 2:
      APP_LOG(APP_LOG_LEVEL_INFO, "Request for:\nTRANSPORT\nsent");
      send(REQUEST_TRANSPORT, "");
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_TRAIN;
      break;
    case 3:
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_UPTIME;
      break;
    case 4:
      layers_data_choices[current_selected_window][current_selected_layer] = ITEM_TIME;
      break;
  }
  
  text_layer_set_text(text_layers[current_selected_window][current_selected_layer], "\nWaiting for data reception...");
  window_stack_pop(true);
}

static void menu_section_4_select_callback(int index, void *ctx) {
  layers_data_choices[current_selected_window][current_selected_layer] = ITEM_BATTERY;
  
  text_layer_set_text(text_layers[current_selected_window][current_selected_layer], "\nWaiting for data reception...");
  window_stack_pop(true);
}

static void menu_window_load(Window *window) {
  // Initializes first menu section's items.
  int num_a_items = 0;
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Location",
    .callback = menu_section_1_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Distance",
    .subtitle = "to a defined point",
    .callback = menu_section_1_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Heading",
    .subtitle = "to a defined point",
    .callback = menu_section_1_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Elevation",
    .callback = menu_section_1_select_callback,
  };
  
  // Initializes second menu section's items.
  num_a_items = 0;  
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Status",
    .callback = menu_section_2_select_callback,
  };
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Temperature",
    .callback = menu_section_2_select_callback,
  };
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Pressure",
    .callback = menu_section_2_select_callback,
  };
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Humidity",
    .callback = menu_section_2_select_callback,
  };
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Wind Speed",
    .callback = menu_section_2_select_callback,
  };
  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Wind Direction",
    .callback = menu_section_2_select_callback,
  };
  
  // Initializes third menu section's items.
  num_a_items = 0;  
  s_third_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Sunrise",
    .callback = menu_section_3_select_callback,
  };
  s_third_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Sunset",
    .callback = menu_section_3_select_callback,
  };
  s_third_menu_items[num_a_items++] = (SimpleMenuItem) {
      .title = "Train Schedule",
    .callback = menu_section_3_select_callback,
  };
  s_third_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Uptime",
    .subtitle = "of the application",
    .callback = menu_section_3_select_callback,
  };
  s_third_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Time",
    .subtitle = "performing activity",
    .callback = menu_section_3_select_callback,
  };
  
  // Initializes fourth menu section's item.
  s_fourth_menu_items[0] = (SimpleMenuItem) {
    .title = "Battery",
    .subtitle = "level of the watch",
    .callback = menu_section_4_select_callback,
  };

  // Initializes each section.
  int num_section = 0;
  s_menu_sections[num_section++] = (SimpleMenuSection) {
    .title = "Location related",
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };
  s_menu_sections[num_section++] = (SimpleMenuSection) {
    .title = "Weather related",
    .num_items = NUM_SECOND_MENU_ITEMS,
    .items = s_second_menu_items,
  };
 s_menu_sections[num_section++] = (SimpleMenuSection) {
    .title = "Time related",
    .num_items = NUM_THIRD_MENU_ITEMS,
    .items = s_third_menu_items,
  };
 s_menu_sections[num_section++] = (SimpleMenuSection) {
    .title = "Battery related",
    .num_items = NUM_FOURTH_MENU_ITEMS,
    .items = s_fourth_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

void menu_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
}

/**
 * Initializes
 */
static void init(void) {
  // Initializes windows array.
  windows[0] = window_1;
  windows[1] = window_2;
  windows[2] = window_3;
  windows[3] = window_4;
  // Initializes text layers array.
  text_layers[0][0] = layer_1_window_1;
  text_layers[0][1] = layer_2_window_1;
  text_layers[1][0] = layer_1_window_2;
  text_layers[1][1] = layer_2_window_2;
  text_layers[2][0] = layer_1_window_3;
  text_layers[2][1] = layer_2_window_3;
  text_layers[3][0] = layer_1_window_4;
  text_layers[3][1] = layer_2_window_4;
  
  // Array of windows unload functions.
  void (*unloadFunctions[4])(Window* w);
  unloadFunctions[0] = window_1_unload;
  unloadFunctions[1] = window_2_unload;
  unloadFunctions[2] = window_3_unload;
  unloadFunctions[3] = window_4_unload;
  
  // Initializes each windows which are in the array.
  for (int i = NUMBER_OF_WINDOWS - 1; i >= 0; --i) {
     // Initializes current layer's user's choices to nothing.
    layers_data_choices[i][0] = -1;
    layers_data_choices[i][1] = -1;
    
    windows[i] = window_create();
    window_set_click_config_provider(windows[i], click_config_provider);
    window_set_window_handlers(windows[i], (WindowHandlers) {
      .load = window_load,
      .unload = unloadFunctions[i],
    });    
  }
  
  // Pushs first windows on the stack to show it at the screen.
  window_stack_push(windows[0], true);
  current_selected_window = 0;
  // No layer is selected yet.
  current_selected_layer = -1;
  
  // Load menu window.
  menu_window = window_create();
  window_set_window_handlers(menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload,
  }); 

  // Subscribe to TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Subscribe to the accelerometer data service
  accel_data_service_subscribe(NUM_ACCEL_SAMPLES, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  app_message_register_inbox_received(received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}
  
static void deinit(void) {
  window_destroy(windows[0]);
}

/**
 * Starts the Pebble app.
 */
int main(void) {  
  init();
  app_event_loop();
  deinit();
}