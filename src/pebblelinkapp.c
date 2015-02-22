#include <pebble.h>
#include <time.h>
#include <string.h>

#define SPLASH_LOADING 1000
  
enum 
KEY { KEY_BUTTON, 
     KEY_VIBRATE, 
     KEY_NAME, 
     KEY_EMAIL,
     KEY_PHONE
    };

#define BUTTON_UP  0
#define BUTTON_SELECT  1
#define BUTTON_DOWN  2
#define MAX_PHONE 20
#define MAX_NAME 60
#define MAX_EMAIL 60
#define CONTACT_KEY 1

static Window *window;
static Window *s_splash_window;
static TextLayer *text_layer;
static TextLayer *name_layer;
static TextLayer *email_layer;
static TextLayer *phone_layer;
static BitmapLayer *s_splash_bitmap_layer; 
AppTimer *timer; 

// text layers 
static TextLayer *s_text_loading_layer;

// bitmap 
static GBitmap *s_splash_bitmap;

typedef struct {
  char  name[MAX_NAME];	       
  char  email[MAX_EMAIL];	  
  char  phone[MAX_PHONE]; 
} personInfo;

/***** Handling App Messages *****/

// For sending functions
static void send(int key, int message)
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter); 

  dict_write_int(iter, key, &message, sizeof(int), true);

  app_message_outbox_send(); 
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) 
{
  personInfo temp; 
  // Get the first pair 
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present 
  while(t != NULL)
  {
    // Process this pair's key
    switch(t->key)
    {
      case KEY_VIBRATE:
      // Trigger vibration 
      text_layer_set_text(text_layer, "Vibrate Activated"); 
      vibes_short_pulse();
      break;
      
      case KEY_NAME:
      strcpy(temp.name, t->value->cstring);
      text_layer_set_text(name_layer, t->value->cstring); 
      break;

      case KEY_EMAIL:
      strcpy(temp.email, t->value->cstring);
      text_layer_set_text(email_layer, t->value->cstring); 
      break;

      case KEY_PHONE:
      strcpy(temp.phone, t->value->cstring);
      text_layer_set_text(phone_layer, t->value->cstring); 
      break;
      
      default:
      APP_LOG(APP_LOG_LEVEL_INFO, "Unknown key: %d", (int)t->key); 
      break;
    }
    // Get next pair, if any
    t = dict_read_next(iterator);
  }
  persist_write_data(CONTACT_KEY, &temp, sizeof(temp));
  printf("The name is %s\n", temp.name); 
  printf("%d", persist_exists(CONTACT_KEY)); 
  vibes_short_pulse(); 
  text_layer_set_text(text_layer, "Contact Recieved.");
}
/********************************* Buttons ************************************/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Establishing link . . . ");

  send(KEY_BUTTON, BUTTON_SELECT);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");

  send(KEY_BUTTON, BUTTON_UP);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");

  send(KEY_BUTTON, BUTTON_DOWN);
}

static void click_config_provider(void *context) {
  // Assign button handlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void inbox_dropped_handler(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message was dropped!"); 
}
static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message that was sent failed!"); 
}
static void outbox_sent_handler(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!"); 
}
void timer_callback(void *data) {
  window_stack_pop(true);
}
/******************************* main_window **********************************/
static void splash_window_load(Window *window){
  // Set a 1000 millisecond to load the splash screen
  app_timer_register(SPLASH_LOADING, (AppTimerCallback) timer_callback, NULL);
  Layer *window_layer = window_get_root_layer(window); 
  GRect window_bounds = layer_get_bounds(window_layer);
  s_splash_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH);
  s_splash_bitmap_layer = bitmap_layer_create(GRect(5, 5, 130, 130));
  bitmap_layer_set_bitmap(s_splash_bitmap_layer, s_splash_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_splash_bitmap_layer));
  s_text_loading_layer = text_layer_create(GRect(5, 120, window_bounds.size.w - 5, 30));
  text_layer_set_font(s_text_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_text_loading_layer, "Establishing Link . . .");
  text_layer_set_text_alignment(s_text_loading_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_text_loading_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_text_loading_layer));  
}
/**
 * This unloads all the layers after splash screen closes. 
 * @param Window: The window of the splash screen
 */
static void splash_window_unload(Window *window){
  text_layer_destroy(s_text_loading_layer);
  gbitmap_destroy(s_splash_bitmap);
  bitmap_layer_destroy(s_splash_bitmap_layer);
}

static void window_load(Window *window) {
  personInfo tempPerson; 
  strcpy(tempPerson.name, ""); 
  strcpy(tempPerson.email, ""); 
  strcpy(tempPerson.phone, ""); 
  
  if (persist_exists(CONTACT_KEY))
  {
    printf("Time to read some previous data"); 
    persist_read_data(CONTACT_KEY, &tempPerson, sizeof(tempPerson));
    text_layer_set_text(name_layer, tempPerson.name); 
    text_layer_set_text(email_layer, tempPerson.email); 
    text_layer_set_text(phone_layer, tempPerson.phone); 
  }
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  name_layer = text_layer_create((GRect) { .origin = { 0, 20 }, .size = { bounds.size.w, 30 } });
  text_layer_set_font(name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
//   text_layer_set_text(name_layer, "Name: Albert Tai");
  text_layer_set_text(name_layer, tempPerson.name);
  text_layer_set_text_alignment(name_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(name_layer, GTextOverflowModeWordWrap);

  email_layer = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, 20 } });
  text_layer_set_font(email_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
//   text_layer_set_text(email_layer, "Email: al@alberttai.com");
  text_layer_set_text(email_layer, tempPerson.email);
  text_layer_set_text_alignment(email_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(email_layer, GTextOverflowModeWordWrap);

  phone_layer = text_layer_create((GRect) { .origin = { 0, 70 }, .size = { bounds.size.w, 20 } });
  text_layer_set_font(phone_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
//   text_layer_set_text(phone_layer, "Phone: 226-239-5218");
  text_layer_set_text(phone_layer, tempPerson.phone);
  text_layer_set_text_alignment(phone_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(phone_layer, GTextOverflowModeWordWrap);
  
  text_layer = text_layer_create((GRect) { .origin = { 0, 90 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(text_layer, "Press Select Button to Send");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  layer_add_child(window_layer, text_layer_get_layer(name_layer));
  layer_add_child(window_layer, text_layer_get_layer(email_layer));
  layer_add_child(window_layer, text_layer_get_layer(phone_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  // Register callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  window = window_create();
  s_splash_window = window_create();
  // Set the click configuration 
  window_set_click_config_provider(window, click_config_provider);
  // This sets up the main screen 
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  // This creates the splash screen 
  window_set_window_handlers(s_splash_window, (WindowHandlers) {
    .load = splash_window_load,
    .unload = splash_window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  window_stack_push(s_splash_window, animated);
}
// Deinitalizing the windows 
static void deinit(void) {
  window_destroy(window);
  window_destroy(s_splash_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
