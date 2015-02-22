#include <pebble.h>
#include <time.h>
  
#define KEY_BUTTON  0
#define KEY_VIBRATE  1

#define BUTTON_UP  0
#define BUTTON_SELECT  1
#define BUTTON_DOWN  2

static Window *window;
static TextLayer *text_layer;

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
      text_layer_set_text(s_text_layer, "Vibrate"); 
      vibes_short_pulse();
      break;
      default:
      APP_LOG(APP_LOG_LEVEL_INFO, "Unknown key: %d", (int)t->key); 
      break;
    }
  }
}
/********************************* Buttons ************************************/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Select");

  send(KEY_BUTTON, BUTTON_SELECT);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");

  send(KEY_BUTTON, BUTTON_UP);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Down");

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
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!"); 
}
static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message sent failed!"); 
}
static void outbox_sent_handler(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!"); 
}

/******************************* main_window **********************************/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Press any button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
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
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
