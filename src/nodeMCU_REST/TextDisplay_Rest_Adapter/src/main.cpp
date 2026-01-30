
#include <Arduino.h>
#include "secrets.h"

#define TEMPLATE_PLACEHOLDER '$'

#if defined(TEXTECKE_ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
  HardwareSerial TextDisplaySerial(2); // UART1 on ESP32
#elif defined(TEXTECKE_ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  HardwareSerial TextDisplaySerial = Serial1; // UART0 on ESP32
#else
  #error "Please define either TEXTECKE_ESP32 or TEXTECKE_ESP8266"
#endif

#include "AsyncJson.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <map>
#include <string>
#include "LittleFS.h"
#include <Wire.h>
#include "logger.h"
#include "database_logger.h"
#include "oauth_handler.h"

#define FRAME_DELIMITER 0xFE
#define ACTION_MESSAGE 0x01
#define ACTION_COLOR 0x02
#define ACTION_SPEED 0x03
#define ACTION_RESET 0x04
#define ACTION_DEBUG 0x05
#define INVALID_HEADER 0x81
#define INVALID_ACTION 0x82
#define BAD_CHKSUM 0x83

using namespace std;
#define MESSAGE_SIZE 190
#define MESSAGE_BUFFER_SIZE 250

// Maximum number of bytes we will accept/forward for an action parameter
#define MAX_PARAM_LEN (MESSAGE_BUFFER_SIZE - 2) // leave room for added space and null

/*Put your SSID & Password*/
// These are defined in the secrets.h (not provided)
const char *ssid = SECRET_SSID;    // Enter SSID here
const char *password = SECRET_KEY; // Enter Password here
const char *hostname = HOST_NAME; // The preferred host name for the server


char current_message[MESSAGE_BUFFER_SIZE] = "Willkommen im ZAM ";
char new_message[MESSAGE_BUFFER_SIZE] = "Willkommen im ZAM ";
char foreground[16] = "white";
char background[16] = "black";

AsyncWebServer server(80);

// Buffer POST bodies per-request (used by the explicit HTTP_POST handler)
std::map<AsyncWebServerRequest *, std::string> postBodyBuffer;

// ==================== EVENT QUEUE FOR DATABASE LOGGING ====================
// Prevents blocking async callbacks with long database operations
// Events are queued in request handlers and processed in loop()

enum EventType {
  EVENT_NONE,
  EVENT_MESSAGE_LOG,
  EVENT_COLOR_LOG
};

// ==================== AUTHORIZATION MIDDLEWARE ====================
#ifdef ENABLE_AUTHORIZATION
/**
 * @brief Validates the Authorization header for incoming requests
 * 
 * Checks if the request contains a valid Bearer token in the Authorization header.
 * Expected format: "Authorization: Bearer <token>"
 * 
 * @param request The incoming HTTP request
 * @return true if authorization is valid or not required, false otherwise
 */
bool isAuthorized(AsyncWebServerRequest *request) {
  // Check if Authorization header exists
  if (!request->hasHeader("Authorization")) {
    LOG_INFO_LN("Authorization failed: No Authorization header present");
    return false;
  }
  
  String authHeader = request->header("Authorization");
  LOG_DEBUG_F("Authorization header: %s\n", authHeader.c_str());
  
  // Expected format: "Bearer <token>"
  if (!authHeader.startsWith("Bearer ")) {
    LOG_INFO_LN("Authorization failed: Invalid format (missing 'Bearer ')");
    return false;
  }
  
  // Extract token (skip "Bearer " prefix)
  String token = authHeader.substring(7);
  token.trim(); // Remove any leading/trailing whitespace
  
  // Compare with configured token
  String expectedToken = String(AUTHORIZATION_TOKEN);
  if (token != expectedToken) {
    LOG_INFO_F("Authorization failed: Invalid token (got: %s)\n", token.c_str());
    return false;
  }
  
  LOG_DEBUG_LN("Authorization successful");
  return true;
}

/**
 * @brief Sends an unauthorized response to the client
 * 
 * @param request The incoming HTTP request to respond to
 */
void sendUnauthorized(AsyncWebServerRequest *request) {
  request->send(401, "application/json; charset=utf-8", 
                "{\"error\":\"Unauthorized - Valid Bearer token required\"}");
}
#endif
// ==================== END AUTHORIZATION MIDDLEWARE ====================

struct DatabaseEvent {
  EventType type;
  char message[MESSAGE_BUFFER_SIZE];
  char foreground_color[16];
  char background_color[16];
  char client_ip[16];
  unsigned long timestamp;
};

#define EVENT_QUEUE_SIZE 10
DatabaseEvent eventQueue[EVENT_QUEUE_SIZE];
uint8_t eventQueueHead = 0;
uint8_t eventQueueTail = 0;
uint8_t eventQueueCount = 0;

/**
 * @brief Queue a database event to be processed in loop()
 * @return true if event was queued successfully, false if queue is full
 */
bool queueDatabaseEvent(const DatabaseEvent &event) {
  if (eventQueueCount >= EVENT_QUEUE_SIZE) {
    LOG_INFO_LN("Event queue full, dropping event!");
    return false;
  }
  
  eventQueue[eventQueueTail] = event;
  eventQueueTail = (eventQueueTail + 1) % EVENT_QUEUE_SIZE;
  eventQueueCount++;
  
  LOG_DEBUG_F("Event queued. Queue size: %u\n", eventQueueCount);
  return true;
}

/**
 * @brief Dequeue and return next database event
 * @return Event with type EVENT_NONE if queue is empty
 */
DatabaseEvent dequeueDatabaseEvent() {
  DatabaseEvent emptyEvent = {EVENT_NONE, "", "", "", "", 0};
  
  if (eventQueueCount == 0) {
    return emptyEvent;
  }
  
  DatabaseEvent event = eventQueue[eventQueueHead];
  eventQueueHead = (eventQueueHead + 1) % EVENT_QUEUE_SIZE;
  eventQueueCount--;
  
  return event;
}

/**
 * @brief Process pending database events
 * Call this from loop() to avoid blocking request handlers
 */
void processPendingDatabaseEvents() {
  DatabaseEvent event = dequeueDatabaseEvent();
  
  while (event.type != EVENT_NONE) {
    switch (event.type) {
      case EVENT_MESSAGE_LOG:
        LOG_DEBUG_F("Processing queued message event: %s from %s\n", event.message, event.client_ip);
        DatabaseLogger::logMessageEvent(event.message, event.client_ip);
        break;
        
      case EVENT_COLOR_LOG:
        LOG_DEBUG_F("Processing queued color event: fg=%s, bg=%s from %s\n", 
                    event.foreground_color, event.background_color, event.client_ip);
        DatabaseLogger::logColorEvent(event.foreground_color, event.background_color, event.client_ip);
        break;
        
      default:
        break;
    }
    
    event = dequeueDatabaseEvent();
  }
}

#define SPECIAL_CHAR 8
uint8_t charmap[SPECIAL_CHAR][2] = {
    {0xA4, 0xE4}, // ä
    {0x84, 0xC4}, // Ä X
    {0xB6, 0xF6}, // ö X
    {0x96, 0xD6}, // Ö
    {0xBC, 0xFC}, // ü X
    {0x9C, 0xDC}, // Ü
    {0x9F, 0xDF}, // ß
    {0xA7, 0xA7}, // §
};


uint8_t calc_checksum(uint8_t message[], uint8_t length)
{
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++)
  {
    checksum += message[i];
  }
  checksum &= 0xFF;
  checksum = 0xFF - checksum;
  LOG_DEBUG_F("check-sum: %02X\n", checksum);
  return checksum;
}

// Log a truncated hex representation of a frame: header, truncated payload, checksum
static void log_truncated_frame(const uint8_t *message_frame, size_t payload_len)
{
  const size_t MAX_PAYLOAD_PRINT = 32;
  char hexbuf[3 * 96];
  size_t off = 0;
  // header bytes (delimiter and length)
  for (size_t i = 0; i < 2 && off < sizeof(hexbuf) - 4; ++i)
  {
    int n = snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02X ", message_frame[i]);
    if (n > 0)
      off += (size_t)n;
  }

  if (payload_len <= MAX_PAYLOAD_PRINT)
  {
    for (size_t i = 0; i < payload_len && off < sizeof(hexbuf) - 4; ++i)
    {
      int n = snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02X ", message_frame[2 + i]);
      if (n > 0)
        off += (size_t)n;
    }
  }
  else
  {
    size_t first = MAX_PAYLOAD_PRINT / 2;
    size_t last = MAX_PAYLOAD_PRINT - first;
    for (size_t i = 0; i < first && off < sizeof(hexbuf) - 4; ++i)
    {
      int n = snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02X ", message_frame[2 + i]);
      if (n > 0)
        off += (size_t)n;
    }
    int n = snprintf(hexbuf + off, sizeof(hexbuf) - off, "...(%u bytes omitted)... ", (unsigned)(payload_len - first - last));
    if (n > 0)
      off += (size_t)n;
    for (size_t i = 0; i < last && off < sizeof(hexbuf) - 4; ++i)
    {
      int m = snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02X ", message_frame[2 + payload_len - last + i]);
      if (m > 0)
        off += (size_t)m;
    }
  }

  // append checksum (last byte)
  if (off < sizeof(hexbuf) - 4)
  {
    int n = snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02X", message_frame[2 + payload_len]);
    if (n > 0)
      off += (size_t)n;
  }
  if (off >= sizeof(hexbuf))
    hexbuf[sizeof(hexbuf) - 1] = '\0';
  else
    hexbuf[off] = '\0';
  LOG_DEBUG_F("Frame (truncated): %s\n", hexbuf);
}



uint8_t get_color_byte(String color_name){
  uint8_t color_byte = -1;
  LOG_DEBUG_F("get_color_byte: color_name: %s \n", color_name.c_str());
  if ( color_name.equals("black") ){
    color_byte = 0x00;
  }
  if ( color_name.equals("red") ){
    color_byte = 0x01;
  }
  if ( color_name.equals("orange") ){
    color_byte = 0x02;
  }
  if ( color_name.equals("yellow") ){
    color_byte = 0x03;
  }
  if ( color_name.equals("green") ){
    color_byte = 0x04;
  }
  if ( color_name.equals("blue") ){
    color_byte = 0x05;
  }
  if ( color_name.equals("indigo") ){
    color_byte = 0x06;
  }
  if ( color_name.equals("violet") ){
    color_byte = 0x07;
  }
  if ( color_name.equals("white")){
    color_byte = 0x08;
  }
  return color_byte;
}

String generate_option(String var, bool fg_option) {

  String template_color = var.substring(var.indexOf('_')+1, var.length());
  LOG_DEBUG_F("generate_option: Checking color: %s against %s \n", template_color, (fg_option ? foreground : background));
  if( (fg_option && template_color.equals(foreground )) || ( !fg_option && template_color.equals(background))){
    LOG_DEBUG_F("generate_option: Returning SELECTED for var %s\n", var.c_str())
    return "selected";
  }
  return " ";
}

String css_processor(const String &var) {
    LOG_DEBUG_F("css_processor var: %s \n", var.c_str());
    if( var == "MARQUEE_FOREGROUND_TOKEN"){
      LOG_DEBUG_F("css_processor var MARQUEE_FOREGROUND_TOKEN: %s \n" , foreground);
      return foreground;
    }
    if( var == "MARQUEE_BACKGROUND_TOKEN"){
      LOG_DEBUG_F("css_processor var MARQUEE_BACKGROUND_TOKEN: %s \n" , background);
      return background;
    }
    LOG_DEBUG_F("css_processor Unknown var: %s \n" , var.c_str());
    return var; // Fehler => leerer String
}

String html_processor(const String &var) {
    LOG_DEBUG_F("html_processor var: %s \n", var.c_str());
    if (var == "MESSAGE_TOKEN"){
      size_t mlen = strlen(current_message);
      String preview = String(current_message);
      const size_t PREVIEW_LEN = 64;
      if (mlen > PREVIEW_LEN) {
        preview = preview.substring(0, PREVIEW_LEN);
        preview.concat("...");
      }
      LOG_DEBUG_F("html_processor var MESSAGE_TOKEN (len=%u): %s \n", (unsigned)mlen, preview.c_str());
      return String(current_message);
    }

    if( var.startsWith("bot")) {
      String back_option = generate_option(var, false);
      LOG_DEBUG_F("html_processor var BACKGROUND_OPTION_TOKEN: %s \n" , back_option.c_str());
      return back_option;
    }
    if( var.startsWith("fot")){
      String front_option = generate_option(var, true);
      LOG_DEBUG_F("html_processor var FOREGROUND_OPTION_TOKEN: %s \n" , front_option.c_str());
      return front_option;
    }

    LOG_DEBUG_F("html_processor Unknown var: %s \n" , var.c_str());
    return var; // Fehler => leerer String
}

/**
 * @brief Takes and action and its parameter, renders the data frame and
 * sends this via the UART to the controlled device.
 * 
 * @param action 
 * @param param 
 */
void render_and_send(const char* action, const char *param) {
    LOG_DEBUG_F("render_and_send: action: %s, param: %s\n", action, param);
    uint8_t message_frame[255];
  size_t frame_idx = 0;
  uint8_t frame_length = 0;
  message_frame[frame_idx++] = FRAME_DELIMITER;

  // Copy parameter into a local, bounded buffer to avoid overruns
  char _param[MESSAGE_BUFFER_SIZE];
  size_t incoming_len = param ? strlen(param) : 0;
  if (incoming_len > MAX_PARAM_LEN) incoming_len = MAX_PARAM_LEN;
  memset(_param, 0x00, sizeof(_param));
  if (incoming_len > 0) memcpy(_param, param, incoming_len);
  _param[incoming_len] = '\0';

    if (strcmp(action, "message") == 0)
    {
      // ensure a single trailing space if there is room
      size_t plen = strlen(_param);
      if (plen < (sizeof(_param) - 1)) {
        _param[plen] = ' ';
        _param[plen+1] = '\0';
        plen++;
      }
      const char *safe_param = _param;
      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_MESSAGE;
      size_t safe_plen = strlen(safe_param);
      // Print a small hex preview once to avoid many blocking Serial calls
      {
        size_t preview = safe_plen > 64 ? 64 : safe_plen;
        char hexbuf[3 * 65];
        size_t off = 0;
        for (size_t i = 0; i < preview; ++i)
        {
          off += snprintf(hexbuf + off, sizeof(hexbuf) - off, "%02x", (unsigned char)safe_param[i]);
          if (i + 1 < preview) {
            if (off < sizeof(hexbuf) - 1) hexbuf[off++] = ' ';
          }
        }
        hexbuf[off < sizeof(hexbuf) ? off : sizeof(hexbuf)-1] = '\0';
        LOG_DEBUG_F("render_and_send: Preparing the message action, param-length: %d, data-preview: %s\n", (int)strlen(param), hexbuf);
      }
      bool escape_mode = false;
      for (size_t i = 0; i < safe_plen; i++)
      {
        if (escape_mode)
        {
          uint8_t subst = 0x20;
          for (uint8_t j = 0; j < SPECIAL_CHAR; j++)
          {
            if (charmap[j][0] == (unsigned char)safe_param[i])
            {
              subst = charmap[j][1];
              break;
            }
          }
          if (frame_idx < sizeof(message_frame) - 1) { message_frame[frame_idx++] = subst; frame_length++; }
          else { break; }
          escape_mode = false;
        }
        else if ((unsigned char)safe_param[i] == 0xC3 || (unsigned char)safe_param[i] == 0xC2)
        {
          escape_mode = true;
        }
        else
        {
          if (frame_idx < sizeof(message_frame) - 1) { message_frame[frame_idx++] = (unsigned char)safe_param[i]; frame_length++; }
          else { break; }
        }
      }
      // finished processing bytes
      message_frame[0x01] = frame_length;
      LOG_DEBUG_F("render_and_send: Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else if (strcmp(action, "color") == 0)
    {
      // Split the string around ":"
      // LHS=foreground, RHS=background.
      // The RHS is optional.
      //
      LOG_DEBUG_LN("render_and_send: Parsing the parameter");

      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_COLOR;
      LOG_DEBUG_LN("Preparing the color action: ");
      char *fg = strtok(_param, ":");
      char *bg = strtok(NULL, ":");
      LOG_DEBUG_F("render_and_send: fg: %s \n", fg);
      if(fg != NULL){
        // copy into fixed-size buffers safely
        strncpy(foreground, fg, sizeof(foreground)-1);
        foreground[sizeof(foreground)-1] = '\0';
        // strupr(foreground);
      }
      message_frame[frame_idx++] = get_color_byte(foreground);
      frame_length++;
      if( bg != NULL){
        LOG_DEBUG_F("render_and_send: bg: %s \n", bg);
        strncpy(background, bg, sizeof(background)-1);
        background[sizeof(background)-1] = '\0';
        // strupr(background);
      }
      message_frame[frame_idx++] = get_color_byte(background);
      frame_length++;
      LOG_DEBUG_LN("");
      message_frame[0x01] = frame_length;
      LOG_DEBUG_F("Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else
    {
      LOG_DEBUG("The action does not match message");
      // request->send(405, "text/plain", "Unknown or unsupported action");
      return;
    }

    uint8_t checksum = calc_checksum(&message_frame[2], frame_length);
    message_frame[frame_idx++] = checksum;

    // Diagnostic: print a truncated hex dump of the frame to avoid blocking Serial
    log_truncated_frame(message_frame, frame_length);

    TextDisplaySerial.write(message_frame, frame_length + 3);
}


void handle_post_form_request(AsyncWebServerRequest *request)
{
  LOG_DEBUG_LN("handle_post_form_request: Received HTTP POST texteck request");
  
  #ifdef ENABLE_AUTHORIZATION
  // Check authorization before processing
  if (!isAuthorized(request)) {
    sendUnauthorized(request);
    return;
  }
  #endif
  
  if (request->method() != HTTP_POST) {
    request->send(405, "text/plain", "Method Not Allowed");
  } else {
    LOG_DEBUG_LN("Starting form processing");
    String tempstr;
      tempstr = request->arg("message");
      LOG_DEBUG_F("handle_post_form_request: freeHeap before processing: %u\n", ESP.getFreeHeap());
      {
        size_t orig_len = tempstr.length();
        if (orig_len > MESSAGE_SIZE) {
          LOG_INFO_F("handle_post_form_request: truncating message from %u to %d characters\n", (unsigned)orig_len, MESSAGE_SIZE);
          tempstr = tempstr.substring(0, MESSAGE_SIZE);
          tempstr.concat("...");
        }
      }
    memset(new_message, 0x00, MESSAGE_BUFFER_SIZE);
    // Copy safely from String to fixed buffer
    tempstr.toCharArray(new_message, MESSAGE_BUFFER_SIZE);
    tempstr.clear();

    tempstr = request->arg("foreground");
    LOG_DEBUG_F("handle_post_form_request: Assigning the foreground: %s\n", tempstr.c_str());
    tempstr.toCharArray(foreground, sizeof(foreground));
    tempstr = request->arg("background");
    LOG_DEBUG_F("handle_post_form_request: Assigning the background: %s\n", tempstr.c_str());
    tempstr.toCharArray(background, sizeof(background));

    LOG_DEBUG_F("message: %s\n", new_message);
    LOG_DEBUG_F("foreground: %s\n", foreground);
    LOG_DEBUG_F("background: %s\n", background);

    tempstr = foreground;
    tempstr.concat(":");
    tempstr.concat(background);

    LOG_DEBUG_F("Rendering the colors to send: %s\n", tempstr.c_str());
    render_and_send("color", tempstr.c_str());

    if(strlen(new_message) > 0) {
      LOG_DEBUG_F("Rendering the message to send: %s\n", new_message);
      render_and_send( "message", new_message);
      memset(current_message, 0x00, MESSAGE_BUFFER_SIZE);
      // copy safely
      strncpy(current_message, new_message, MESSAGE_BUFFER_SIZE - 1);
      current_message[MESSAGE_BUFFER_SIZE - 1] = '\0';
      LOG_DEBUG_F("Current Message: %s\n", current_message);
    }

    LOG_DEBUG_F("handle_post_form_request: freeHeap after processing: %u\n", ESP.getFreeHeap());
    
    // Queue database events to be processed in loop() to avoid blocking async handler
    #ifdef ENABLE_DATABASE_LOGGING
        String forwardIP = request->header("X-Forwarded-For");
        String clientIP = "";
        if (forwardIP.length() > 0) {
          clientIP = forwardIP;
        } else {
          clientIP = request->client()->remoteIP().toString();
        }
        
        // Queue message event
        DatabaseEvent msgEvent = {EVENT_MESSAGE_LOG, "", "", "", "", millis()};
        strncpy(msgEvent.message, current_message, sizeof(msgEvent.message) - 1);
        msgEvent.message[sizeof(msgEvent.message) - 1] = '\0';
        strncpy(msgEvent.client_ip, clientIP.c_str(), sizeof(msgEvent.client_ip) - 1);
        msgEvent.client_ip[sizeof(msgEvent.client_ip) - 1] = '\0';
        queueDatabaseEvent(msgEvent);
        
        // Queue color event
        DatabaseEvent colorEvent = {EVENT_COLOR_LOG, "", "", "", "", millis()};
        strncpy(colorEvent.foreground_color, foreground, sizeof(colorEvent.foreground_color) - 1);
        colorEvent.foreground_color[sizeof(colorEvent.foreground_color) - 1] = '\0';
        strncpy(colorEvent.background_color, background, sizeof(colorEvent.background_color) - 1);
        colorEvent.background_color[sizeof(colorEvent.background_color) - 1] = '\0';
        strncpy(colorEvent.client_ip, clientIP.c_str(), sizeof(colorEvent.client_ip) - 1);
        colorEvent.client_ip[sizeof(colorEvent.client_ip) - 1] = '\0';
        queueDatabaseEvent(colorEvent);
    #endif

    LOG_DEBUG_LN("Sending the main page back\n");
    request->send(LittleFS, "/index.min.html", String(), false, html_processor); 
    tempstr.clear();
  }
}

/**
 * @brief Handles the REST API requests.
 * 
 */
void handle_rest_request(AsyncWebServerRequest *request, JsonVariant &docVar)
{
  LOG_DEBUG_LN("Received REST POST request");
    String result = "";
    // Support new-style payloads: { "text": "..." }
    if (docVar.containsKey("text") && docVar["text"].is<const char*>()) {
      String param = String(docVar["text"].as<const char*>());
      if (param.length() > MESSAGE_SIZE) {
        LOG_DEBUG_F("handle_rest_request: The message is greater than %d characters. Truncating\n", MESSAGE_SIZE);
        param = param.substring(0, MESSAGE_SIZE);
        param.concat("...");
      }
      LOG_DEBUG_F("handle_rest_request: Calling render and send for message, param: %s", param.c_str());
      memset(new_message, 0x00, MESSAGE_BUFFER_SIZE);
      param.toCharArray(new_message, MESSAGE_BUFFER_SIZE);
      LOG_DEBUG_F("handle_rest_request: freeHeap before render: %u\n", ESP.getFreeHeap());
      memset(current_message, 0x00, MESSAGE_BUFFER_SIZE);
      strncpy(current_message, new_message, MESSAGE_BUFFER_SIZE - 1);
      current_message[MESSAGE_BUFFER_SIZE - 1] = '\0';
      render_and_send("message", current_message);
      LOG_DEBUG_F("handle_rest_request: freeHeap after render: %u\n", ESP.getFreeHeap());
      #ifdef ENABLE_DATABASE_LOGGING
          String clientIP = request->client()->remoteIP().toString();
          DatabaseEvent msgEvent = {EVENT_MESSAGE_LOG, "", "", "", "", millis()};
          strncpy(msgEvent.message, current_message, sizeof(msgEvent.message) - 1);
          msgEvent.message[sizeof(msgEvent.message) - 1] = '\0';
          strncpy(msgEvent.client_ip, clientIP.c_str(), sizeof(msgEvent.client_ip) - 1);
          msgEvent.client_ip[sizeof(msgEvent.client_ip) - 1] = '\0';
          queueDatabaseEvent(msgEvent);
      #endif
    } else {
      // New-style color fields: handle fg/bg alongside legacy action/param
      bool hasFgField = docVar.containsKey("fg") && docVar["fg"].is<const char*>();
      bool hasBgField = docVar.containsKey("bg") && docVar["bg"].is<const char*>();
      if (hasFgField || hasBgField) {
        if (hasFgField) {
          const char *fg = docVar["fg"].as<const char *>();
          if (strlen(fg) >= sizeof(foreground)) {
            request->send(413, "application/json; charset=utf-8", "{\"error\":\"'fg' too long\"}");
            return;
          }
          strncpy(foreground, fg, sizeof(foreground)-1);
          foreground[sizeof(foreground)-1] = '\0';
        }
        if (hasBgField) {
          const char *bg = docVar["bg"].as<const char *>();
          if (strlen(bg) >= sizeof(background)) {
            request->send(413, "application/json; charset=utf-8", "{\"error\":\"'bg' too long\"}");
            return;
          }
          strncpy(background, bg, sizeof(background)-1);
          background[sizeof(background)-1] = '\0';
        }
        String temp = String(foreground);
        temp.concat(":");
        temp.concat(background);
        LOG_DEBUG_F("handle_rest_request: Applying colors %s\n", temp.c_str());
        render_and_send("color", temp.c_str());
        #ifdef ENABLE_DATABASE_LOGGING
            String clientIP = request->client()->remoteIP().toString();
            DatabaseEvent colorEvent = {EVENT_COLOR_LOG, "", "", "", "", millis()};
            strncpy(colorEvent.foreground_color, foreground, sizeof(colorEvent.foreground_color) - 1);
            colorEvent.foreground_color[sizeof(colorEvent.foreground_color) - 1] = '\0';
            strncpy(colorEvent.background_color, background, sizeof(colorEvent.background_color) - 1);
            colorEvent.background_color[sizeof(colorEvent.background_color) - 1] = '\0';
            strncpy(colorEvent.client_ip, clientIP.c_str(), sizeof(colorEvent.client_ip) - 1);
            colorEvent.client_ip[sizeof(colorEvent.client_ip) - 1] = '\0';
            queueDatabaseEvent(colorEvent);
        #endif
      }

      // Fallback: legacy format { "action": "xxx", "param": "yyy" }
      String action = docVar["action"];
      String param = docVar["param"];
      if ( action.equals("message")){
        if(param.length() > MESSAGE_SIZE){
            LOG_DEBUG_F("handle_rest_request: The message is greater than %d characters. Truncating\n", MESSAGE_SIZE);
            param = param.substring(0, MESSAGE_SIZE);
            param.concat("...");
        }
        LOG_DEBUG_F("handle_rest_request: Calling render and send for: %s, param: %s", action.c_str(), param.c_str());
        memset(new_message, 0x00, MESSAGE_BUFFER_SIZE);
        // Copy safely from String into fixed buffer
        param.toCharArray(new_message, MESSAGE_BUFFER_SIZE);
        LOG_DEBUG_F("handle_rest_request: freeHeap before render: %u\n", ESP.getFreeHeap());
        // copy into current_message safely and render
        memset(current_message, 0x00, MESSAGE_BUFFER_SIZE);
        strncpy(current_message, new_message, MESSAGE_BUFFER_SIZE - 1);
        current_message[MESSAGE_BUFFER_SIZE - 1] = '\0';
        render_and_send(action.c_str(), current_message);
        LOG_DEBUG_F("handle_rest_request: freeHeap after render: %u\n", ESP.getFreeHeap());
      } else {
        LOG_DEBUG_F("handle_rest_request: Calling render and send for: %s, param: %s. Existing fg: %s, bg: %s \n", 
          action.c_str(), param.c_str(), foreground, background);
        if (param.startsWith(":")){
          String tmpstr = String(foreground);
          tmpstr.concat(param);
          LOG_DEBUG_F("Color code to be passed to render_and_send %s \n", tmpstr.c_str());
          render_and_send(action.c_str(), tmpstr.c_str());
        } else {
          LOG_DEBUG_F("Color code to be passed to render_and_send %s \n", param.c_str());
          render_and_send(action.c_str(), param.c_str());
        }
      }
    }

    String response;
    LOG_DEBUG_F("Current message %s \n", current_message);
    serializeJson(docVar, response);
    LOG_DEBUG_F("REST Response: %s \n", response.c_str());
    LOG_DEBUG_LN("REST Response: OK");


    request->send(200, "text/plain; charset=utf-8", "OK");
  
}

void handle_notFound(AsyncWebServerRequest *request)
{
  LOG_DEBUG_F("handle_notFound: Unable to handle the request: %s\n", request->url().c_str());
  request->send(404, "text/plain", "The texteck resource was not found url");
}


void setup_system_resources()
{
  LOG_INIT(115200);
  TextDisplaySerial.begin(9600);
  delay(100);

  if(!LittleFS.begin()){
      LOG_ERROR_LN("An Error has occurred while mounting LittleFS");
      return;
  }
}

void setup_wifi_and_network()
{
  LOG_DEBUG_F("Connecting to %s \n", ssid);
  // connect to your local wi-fi network
  WiFi.mode(WIFI_STA);

  LOG_DEBUG_F("Default hostname: %s\n", WiFi.getHostname());
  WiFi.mode(WIFI_AP_STA); // or any other mode
  WiFi.setHostname(HOST_NAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    LOG_DEBUG_S(".");
  }
  LOG_INFO_LN("");
  LOG_INFO_LN("WiFi connected..!");
  LOG_INFO_F("This hostname: %s\n", WiFi.getHostname());
  LOG_INFO("Got IP: ");
  LOG_INFO_LN(WiFi.localIP());
}

void setup_rest_api()
{
  // -- REST
  // POST endpoint: accept JSON body and call handle_rest_request
  server.on("/api/v1/message", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // onRequest - response is handled after body is parsed in onBody
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      LOG_INFO_LN("Received REST POST message request");
      
      #ifdef ENABLE_AUTHORIZATION
      // Check authorization on first chunk
      if (index == 0 && !isAuthorized(request)) {
        sendUnauthorized(request);
        postBodyBuffer.erase(request);
        return;
      }
      #endif
      
      std::string &buf = postBodyBuffer[request];
      if (index == 0) buf.clear();
      buf.append((const char *)data, len);
      if (index + len == total) {
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, buf.c_str(), buf.size());
        if (err) {
          request->send(400, "application/json; charset=utf-8", "{\"error\":\"Invalid JSON\"}");
        } else {
          // Validate presence and byte-length of 'text' (max 200 bytes)
          if (!doc.containsKey("text") || !doc["text"].is<const char*>()) {
            request->send(400, "application/json; charset=utf-8", "{\"error\":\"Missing or invalid 'text' field\"}");
          } else {
            const char *txt = doc["text"].as<const char *>();
            size_t txt_len = strlen(txt); // bytes in UTF-8
            if (txt_len > 200) {
              request->send(413, "application/json; charset=utf-8", "{\"error\":\"'text' too long (max 200 bytes)\"}");
            } else {
              JsonVariant var = doc.as<JsonVariant>();
              handle_rest_request(request, var);
            }
          }
        }
        postBodyBuffer.erase(request);
      }
    }
  );
    // GET endpoint: return the current message as JSON
    server.on("/api/v1/message", HTTP_GET, [](AsyncWebServerRequest *request) {
      LOG_INFO_LN("Received REST GET message request");
      
      #ifdef ENABLE_AUTHORIZATION
      if (!isAuthorized(request)) {
        sendUnauthorized(request);
        return;
      }
      #endif
      
      String response;
      DynamicJsonDocument doc(512);
      doc["message"] = current_message;
      serializeJson(doc, response);
      request->send(200, "application/json; charset=utf-8", response);
    });

    // Color endpoints
    // POST /api/v1/color - accept JSON with at least one of 'fg' or 'bg'
    server.on("/api/v1/color", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // onRequest - response handled in body callback
      },
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        LOG_INFO_LN("Received REST POST color request");
        
        #ifdef ENABLE_AUTHORIZATION
        // Check authorization on first chunk
        if (index == 0 && !isAuthorized(request)) {
          sendUnauthorized(request);
          postBodyBuffer.erase(request);
          return;
        }
        #endif
        
        std::string &buf = postBodyBuffer[request];
        if (index == 0) buf.clear();
        buf.append((const char *)data, len);
        if (index + len == total) {
          DynamicJsonDocument doc(512);
          DeserializationError err = deserializeJson(doc, buf.c_str(), buf.size());
          if (err) {
            request->send(400, "application/json; charset=utf-8", "{\"error\":\"Invalid JSON\"}");
          } else {
            bool hasFg = doc.containsKey("fg") && doc["fg"].is<const char*>();
            bool hasBg = doc.containsKey("bg") && doc["bg"].is<const char*>();
            if (!hasFg && !hasBg) {
              request->send(400, "application/json; charset=utf-8", "{\"error\":\"Must provide 'fg' or 'bg'\"}");
            } else {
              // Validate lengths (max 15 bytes to fit into foreground/background buffers)
              if (hasFg) {
                const char *fg = doc["fg"].as<const char *>();
                if (strlen(fg) >= sizeof(foreground)) {
                  request->send(413, "application/json; charset=utf-8", "{\"error\":\"'fg' too long (max 15 bytes)\"}");
                  postBodyBuffer.erase(request);
                  return;
                }
                strncpy(foreground, fg, sizeof(foreground)-1);
                foreground[sizeof(foreground)-1] = '\0';
              }
              if (hasBg) {
                const char *bg = doc["bg"].as<const char *>();
                if (strlen(bg) >= sizeof(background)) {
                  request->send(413, "application/json; charset=utf-8", "{\"error\":\"'bg' too long (max 15 bytes)\"}");
                  postBodyBuffer.erase(request);
                  return;
                }
                strncpy(background, bg, sizeof(background)-1);
                background[sizeof(background)-1] = '\0';
              }
              // Apply colors to display
              String tempstr = String(foreground);
              tempstr.concat(":");
              tempstr.concat(background);
              render_and_send("color", tempstr.c_str());
              #ifdef ENABLE_DATABASE_LOGGING
                  String clientIP = request->client()->remoteIP().toString();
                  DatabaseEvent colorEvent = {EVENT_COLOR_LOG, "", "", "", "", millis()};
                  strncpy(colorEvent.foreground_color, foreground, sizeof(colorEvent.foreground_color) - 1);
                  colorEvent.foreground_color[sizeof(colorEvent.foreground_color) - 1] = '\0';
                  strncpy(colorEvent.background_color, background, sizeof(colorEvent.background_color) - 1);
                  colorEvent.background_color[sizeof(colorEvent.background_color) - 1] = '\0';
                  strncpy(colorEvent.client_ip, clientIP.c_str(), sizeof(colorEvent.client_ip) - 1);
                  colorEvent.client_ip[sizeof(colorEvent.client_ip) - 1] = '\0';
                  queueDatabaseEvent(colorEvent);
              #endif
              request->send(200, "application/json; charset=utf-8", "{\"status\":\"ok\"}");
            }
          }
          postBodyBuffer.erase(request);
        }
      }
    );

    // GET /api/v1/color - return current fg/bg
    server.on("/api/v1/color", HTTP_GET, [](AsyncWebServerRequest *request) {
      LOG_INFO_LN("Received REST GET color request");
      
      #ifdef ENABLE_AUTHORIZATION
      if (!isAuthorized(request)) {
        sendUnauthorized(request);
        return;
      }
      #endif
      
      String response;
      DynamicJsonDocument doc(256);
      doc["fg"] = foreground;
      doc["bg"] = background;
      serializeJson(doc, response);
      request->send(200, "application/json; charset=utf-8", response);
    });
}

void setup_html_ui()
{
  // -- HTTP
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { 
                  request->send(LittleFS, "/index.min.html", String(), false, html_processor); 
                });

  // Register specific handler for CSS that needs processor
  server.on("/res/style.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
                { 
                  request->send(LittleFS, "/res/style.min.css", String(), false, css_processor); 
                });

  // Serve all other static resources from /res/ directory
  server.serveStatic("/res/", LittleFS, "/res/");

  server.on("/", HTTP_POST, handle_post_form_request);

  server.onNotFound(handle_notFound);
}

void setup()
{
  setup_system_resources();
  setup_wifi_and_network();
  setup_rest_api();
  setup_html_ui();

  server.begin();
  LOG_DEBUG_LN("HTTP server started");
  
  DatabaseLogger::begin();

  // Initialize display with default values
  render_and_send("message", current_message);
  String tempstr = foreground;
  tempstr.concat(":");
  tempstr.concat(background);
  render_and_send("color", tempstr.c_str());
}


void loop()
{
  // Process any pending database events without blocking
  processPendingDatabaseEvents();
  
  // Yield to other tasks and prevent watchdog timeout
  delay(10);
}
