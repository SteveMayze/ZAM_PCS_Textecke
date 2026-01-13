
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
#include "LittleFS.h"
#include <Wire.h>
#include "logger.h"

#define FRAME_DELIMITER 0xFE
#define ACTION_MESSAGE 0x01
#define ACTION_COLOUR 0x02
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



uint8_t get_colour_byte(String colour_name){
  uint8_t colour_byte = -1;
  LOG_DEBUG_F("get_colour_byte: colour_name: %s \n", colour_name.c_str());
  if ( colour_name.equals("black") ){
    colour_byte = 0x00;
  }
  if ( colour_name.equals("red") ){
    colour_byte = 0x01;
  }
  if ( colour_name.equals("orange") ){
    colour_byte = 0x02;
  }
  if ( colour_name.equals("yellow") ){
    colour_byte = 0x03;
  }
  if ( colour_name.equals("green") ){
    colour_byte = 0x04;
  }
  if ( colour_name.equals("blue") ){
    colour_byte = 0x05;
  }
  if ( colour_name.equals("indigo") ){
    colour_byte = 0x06;
  }
  if ( colour_name.equals("violet") ){
    colour_byte = 0x07;
  }
  if ( colour_name.equals("white")){
    colour_byte = 0x08;
  }
  return colour_byte;
}

String generate_option(String var, bool fg_option) {

  String template_colour = var.substring(var.indexOf('_')+1, var.length());
  // LOG_DEBUG_F("generate_option: Checking colour: %s against %s \n", template_colour, (fg_option ? foreground : background));
  if( (fg_option && template_colour.equals(foreground )) || ( !fg_option && template_colour.equals(background))){
    LOG_DEBUG_F("generate_option: Reurning SELECTED for var %s\n", var.c_str())
    return "selected";
  }
  return " ";
}

String css_processor(const String &var) {
    // LOG_DEBUG_F("css_processor var: %s \n", var.c_str());
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
    // LOG_DEBUG_F("html_processor var: %s \n", var.c_str());
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
      // LOG_DEBUG_F("html_processor var BACKGROUND_OPTION_TOKEN: %s \n" , back_option.c_str());
      return back_option;
    }
    if( var.startsWith("fot")){
      String front_option = generate_option(var, true);
      // LOG_DEBUG_F("html_processor var FOREGROUND_OPTION_TOKEN: %s \n" , front_option.c_str());
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
    else if (strcmp(action, "colour") == 0||strcmp(action, "color") == 0)
    {
      // Split the string around ":"
      // LHS=foreground, RHS=background.
      // The RHS is optional.
      //
      LOG_DEBUG_LN("render_and_send: Parsing the parameter");

      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_COLOUR;
      LOG_DEBUG_LN("Preparing the colour action: ");
      char *fg = strtok(_param, ":");
      char *bg = strtok(NULL, ":");
      LOG_DEBUG_F("render_and_send: fg: %s \n", fg);
      if(fg != NULL){
        // copy into fixed-size buffers safely
        strncpy(foreground, fg, sizeof(foreground)-1);
        foreground[sizeof(foreground)-1] = '\0';
        // strupr(foreground);
      }
      message_frame[frame_idx++] = get_colour_byte(foreground);
      frame_length++;
      if( bg != NULL){
        LOG_DEBUG_F("render_and_send: bg: %s \n", bg);
        strncpy(background, bg, sizeof(background)-1);
        background[sizeof(background)-1] = '\0';
        // strupr(background);
      }
      message_frame[frame_idx++] = get_colour_byte(background);
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
  LOG_DEBUG_LN("handle_post_form_request: Received HTTP POST textecke request");
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

    LOG_DEBUG_F("Rendering the colours to send: %s\n", tempstr.c_str());
    render_and_send("colour", tempstr.c_str());

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
        LOG_DEBUG_F("Colour code to be passed to render_and_send %s \n", tmpstr.c_str());
        render_and_send(action.c_str(), tmpstr.c_str());
      } else {
        LOG_DEBUG_F("Colour code to be passed to render_and_send %s \n", param.c_str());
        render_and_send(action.c_str(), param.c_str());
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
  LOG_DEBUG_F("handle_notFound: Unable to handle the request: %s", request->url().c_str());
  request->send(404, "text/plain", "The textecke resource was not found url");
}


void setup()
{

  LOG_INIT(115200);
  TextDisplaySerial.begin(9600);
  delay(100);
 
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

  if(!LittleFS.begin()){
      LOG_ERROR_LN("An Error has occurred while mounting LittleFS");
      return;
  }
 
  // -- REST
  // server.on("/api/v1/message", HTTP_POST, handle_rest_request);
  AsyncCallbackJsonWebHandler *handle_json = new AsyncCallbackJsonWebHandler("/api/v1/message", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Pass the parsed JsonVariant directly to the request handler to avoid
    // holding a large static JsonDocument in RAM.
    handle_rest_request(request, json);
    }); 

    server.addHandler(handle_json);
    // -- HTTP
    server.on("/textecke", HTTP_GET, [](AsyncWebServerRequest *request)
                  { 
                    request->send(LittleFS, "/index.min.html", String(), false, html_processor); 
                  });

    server.on("/res/style.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
                  { 
                    request->send(LittleFS, "/res/style.min.css", String(), false, css_processor); 
                  });

    server.on("/res/ZAM_ot-Logo-wt.png", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(LittleFS, "/res/ZAM_ot-Logo-wt.png", "image/png"); });

    server.on("/textecke", HTTP_POST, handle_post_form_request);

    server.onNotFound(handle_notFound);

    server.begin();
    LOG_DEBUG_LN("HTTP server started");

    render_and_send("message", current_message);
    String tempstr = foreground;
    tempstr.concat(":");
    tempstr.concat(background);
    // LOG_DEBUG_F("Rendering the colours to send: %s\n", tempstr.c_str());
    render_and_send("colour", tempstr.c_str());
}
void loop()
{

}
