#include <ArduinoJson.h>
#include "secrets.h"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
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
#define MESSAGE_SIZE 200

/*Put your SSID & Password*/
// These are defined in the secrets.h (not provided)
const char *ssid = SECRET_SSID;    // Enter SSID here
const char *password = SECRET_KEY; // Enter Password here
const char *hostname = HOST_NAME; // The preferred host name for the server

char current_message[MESSAGE_SIZE] = "Willkommen im ZAM ";
char new_message[MESSAGE_SIZE] = "Willkommen im ZAM ";
char foreground[16] = "WHITE";
char background[16] = "BLACK";

StaticJsonDocument<1024> doc;

AsyncWebServer server(80);

String send_response()
{
  String ptr = "{}\n";
  return ptr;
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


uint8_t get_colour_byte(String colour_name){
  uint8_t colour_byte = -1;
  if ( colour_name.equals("BLACK") ){
    colour_byte = 0x00;
  }
  if ( colour_name.equals("RED") ){
    colour_byte = 0x01;
  }
  if ( colour_name.equals("ORANGE")){
    colour_byte = 0x02;
  }
  if ( colour_name.equals("YELLOW")){
    colour_byte = 0x03;
  }
  if ( colour_name.equals("GREEN")){
    colour_byte = 0x04;
  }
  if ( colour_name.equals("BLUE")){
    colour_byte = 0x05;
  }
  if ( colour_name.equals("INDIGO")){
    colour_byte = 0x06;
  }
  if ( colour_name.equals("VIOLET")){
    colour_byte = 0x07;
  }
  if ( colour_name.equals("WHITE")){
    colour_byte = 0x08;
  }
  return colour_byte;
}

String generate_option(bool fg_option) {
/*
<option id=\"back_black\" value=\"BLACK\" selected>black</option>\
<option id=\"back_red\" value=\"RED\">red</option>\
<option id=\"back_orange\" value=\"ORANGE\">orange</option>\
<option id=\"back_yellow\" value=\"YELLOW\">yellow</option>\
<option id=\"back_green\" value=\"GREEN\">green</option>\
<option id=\"back_blue\" value=\"BLUE\">blue</option>\
<option id=\"back_violet\" value=\"VIOLET\">violet</option>\
<option id=\"back_white\" value=\"WHITE\">white</option>\
*/
  String data[] = {"black", "red", "orange", "yellow", "green", "blue", "violet", "white", };
  char line[100];
  String opt_template = String( "<option id=\"%s_%s\" value=\"%s\" %s>%s</option>\n");
  String option_result = String();
  char layer[6];

  String layer_colour;
  if( fg_option) {
    strcpy(layer, "fore");
    layer_colour = foreground;
  }
  else
  {  
    strcpy(layer, "back");
    layer_colour = background;
  }
  LOG_DEBUG_F("Layer colour: %s\n", layer_colour.c_str());
  String selected;
  for(uint8_t i = 0; i < 8; i++){
    String value = String(data[i]);
    value.toUpperCase();

    if( value.equals(layer_colour)) 
      selected = "selected";
    else
      selected = "";

    sprintf(line, opt_template.c_str(), layer, data[i].c_str(),value.c_str(), selected, data[i].c_str());
    option_result.concat(line);
  }
  LOG_DEBUG_F("Generated option: %s\n", option_result.c_str());
  return option_result;
}

String processor(const String &var) {
    LOG_DEBUG_LN("Starting form rendering");
    String back_option = generate_option(false);
    String front_option = generate_option(true);
    if (var == "MESAGE_TOKEN"){
      return current_message;
    }
    if( var == "MARQUEE_FOREGROUND_TOKEN"){
      return foreground;
    }
    if( var == "MARQUEE_BACKGROUND_TOKEN"){
      return background;
    }
    if( var == "BACKGROUND_OPTION_TOKEN"){
      return back_option;
    }
    if( var == "FOREGROUND_OPTION_TOKEN"){
      return front_option;
    }
    return String(); // Fehler => leerer String
}

/**
 * @brief Takes and action and its parameter, renders the data frame and
 * sends this via the UART to the controlled device.
 * 
 * @param action 
 * @param param 
 */
void render_and_send(AsyncWebServerRequest *request, String action, String param) {
    LOG_DEBUG_F("action: %s, param: %s\n", action.c_str(), param.c_str());
    uint8_t message_frame[255];
    uint8_t frame_idx = 0;
    uint8_t frame_length = 0;
    message_frame[frame_idx++] = FRAME_DELIMITER;
    if (action.equals("message"))
    {
      param.concat(" ");
      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_MESSAGE;
      LOG_DEBUG("Preparing the message action: ");
      bool escape_mode = false;
      for (uint8_t i = 0; i < param.length(); i++)
      {
        LOG_DEBUG_F("%02x ", param[i]);
        if (escape_mode)
        {
          uint8_t subst = 0x20;
          for (uint8_t j = 0; j < SPECIAL_CHAR; j++)
          {
            if (charmap[j][0] == param[i])
            {
              subst = charmap[j][1];
              break;
            }
          }
          message_frame[frame_idx++] = subst;
          frame_length++;
          escape_mode = false;
        }
        else if (param[i] == 0xC3 || param[i] == 0xC2)
        {
          escape_mode = true;
        }
        else
        {
          message_frame[frame_idx++] = param[i];
          frame_length++;
        }
      }
      LOG_DEBUG_LN("");
      message_frame[0x01] = frame_length;
      LOG_DEBUG_F("Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else if (action.equals("colour")||action.equals("color"))
    {
      // Split the string around ":"
      // LHS=foreground, RHS=background.
      // The RHS is optional.
      //
      param.toUpperCase();
      int sep = param.indexOf(':');
      if ( sep > 0){
        strcpy(foreground, param.substring(0, sep).c_str());
        strcpy(background, param.substring(sep+1).c_str());
      } else {
        strcpy(foreground, param.c_str());
      }

      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_COLOUR;
      LOG_DEBUG("Preparing the colour action: ");

      message_frame[frame_idx++] = get_colour_byte(foreground);
      frame_length++;
      if (strlen(background)>0){
        message_frame[frame_idx++] = get_colour_byte(background);
        frame_length++;
      }
      LOG_DEBUG_LN("");
      message_frame[0x01] = frame_length;
      LOG_DEBUG_F("Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else
    {
      LOG_DEBUG("The action does not match message");
      request->send(405, "text/plain", "Unknown or unsupported action");
      return;
    }

    uint8_t checksum = calc_checksum(&message_frame[2], frame_length);
    message_frame[frame_idx++] = checksum;
    for (uint8_t i = 0; i < frame_length + 3; i++)
    {
      LOG_DEBUG_F("%02X ", message_frame[i]);
    }
    LOG_DEBUG_LN("");
    Serial1.write(message_frame, frame_length + 3);
    request->send(SPIFFS, "/index.html", String(), false, processor); 

}



void handle_post_form_request(AsyncWebServerRequest *request)
{
  LOG_DEBUG_LN("Received HTTP POST textecke request");
  if (request->method() != HTTP_POST) {
    request->send(405, "text/plain", "Method Not Allowed");
  } else {
    LOG_DEBUG_LN("Starting form processing");
    String tempstr;
    tempstr = request->arg("message");
    LOG_DEBUG_F("Assigning the message: %s\n", tempstr.c_str());
    if(tempstr.length() > MESSAGE_SIZE-10){
        LOG_DEBUG_F("The message is greater than %d characters. Truncating\n", MESSAGE_SIZE-10);
        tempstr = tempstr.substring(0, MESSAGE_SIZE-10);
        tempstr.concat("...");
    }
    strcpy(new_message, tempstr.c_str());
    tempstr = request->arg("foreground");
    LOG_DEBUG_F("Assigning the foreground: %s\n", tempstr.c_str());
    strcpy(foreground, tempstr.c_str());
    tempstr = request->arg("background");
    LOG_DEBUG_F("Assigning the background: %s\n", tempstr.c_str());
    strcpy(background, tempstr.c_str());

    LOG_DEBUG_F("message: %s\n", new_message);
    LOG_DEBUG_F("foreground: %s\n", foreground);
    LOG_DEBUG_F("background: %s\n", background);

    tempstr = foreground;
    tempstr.concat(":");
    tempstr.concat(background);

    LOG_DEBUG_F("Rendering the colours to send: %s\n", tempstr.c_str());
    render_and_send(request, "colour", tempstr.c_str());

    LOG_DEBUG_F("Rendering the message to send: %s\n", new_message);
    if(strlen(new_message) > 0){
      render_and_send(request, "message", new_message);
      strcpy(current_message, new_message);
      LOG_DEBUG_F("Current Message %s\n", current_message);
    }

    request->send(SPIFFS, "/index.html", String(), false, processor); 
  }
}

/**
 * @brief Handles the REST API requests.
 * 
 */
void handle_rest_request(AsyncWebServerRequest *request)
{
  LOG_DEBUG_LN("Received REST POST request");

  if (request->method() != HTTP_POST)
  {
    request->send(405, "text/plain", "Method Not Allowed");
  }
  else
  {
    String result = "";

    if (request->hasArg("plain") == false)
    { // Check if body received
      request->send(200, "text/plain", "The body must be of type application/json");
      return;
    }
    String content = request->arg("plain");
    LOG_DEBUG_LN("Content: " + content);
    DeserializationError error = deserializeJson(doc, content);
    if (error)
    {
      LOG_DEBUG(F("deserializeJson() failed: "));
      LOG_DEBUG_LN(error.f_str());
      request->send(405, "text/plain", "bad content");
      return;
    }

    String action = doc["action"];
    String param = doc["param"];
    if(param.length() > MESSAGE_SIZE){
        LOG_DEBUG_F("The message is greater than %d characters. Truncating\n", MESSAGE_SIZE);
        param = param.substring(0, MESSAGE_SIZE-10);
        param.concat("...");
    }
    strcpy(current_message, param.c_str());
    render_and_send(request, action, param);

    request->send(200, "text/plain; charset=utf-8", "OK");
  }
}


void handle_notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}


void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial1.begin(9600);
  delay(100);
 
  LOG_DEBUG_LN("Connecting to ");
  LOG_DEBUG_LN(ssid);
  // connect to your local wi-fi network
  WiFi.mode(WIFI_STA);

  LOG_DEBUG_F("Default hostname: %s\n", WiFi.hostname().c_str());
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    LOG_DEBUG(".");
  }
  LOG_DEBUG_LN("");
  LOG_DEBUG_LN("WiFi connected..!");
  LOG_DEBUG_F("This hostname: %s\n", WiFi.hostname().c_str());
  LOG_DEBUG("Got IP: ");
  LOG_DEBUG_LN(WiFi.localIP());

if(!LittleFS.begin()){
      LOG_DEBUG_LN("An Error has occurred while mounting LittleFS");
      return;
  }
 
  // -- REST
  server.on("/api/v1/message", HTTP_POST, handle_rest_request);

  // -- HTTP
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { 
                  request->send(SPIFFS, "/index.html", String(), false, processor); 
                });

  // server.on("/textecke", HTTP_POST, handle_post_form_request);
  server.on("/textecke", HTTP_POST, handle_post_form_request);
  server.onNotFound(handle_notFound);

  server.begin();
  LOG_DEBUG_LN("HTTP server started");

  //  render_and_send("message", current_message);
    String tempstr = foreground;
    tempstr.concat(":");
    tempstr.concat(background);
    LOG_DEBUG_F("Rendering the colours to send: %s\n", tempstr.c_str());
  //  render_and_send("colour", tempstr.c_str());
}
void loop()
{
  ;
}
