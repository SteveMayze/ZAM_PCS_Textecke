#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define TRACE(...) Serial.printf(__VA_ARGS__)

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
// typedef struct _Data_Frame {
//   // uint8_t data_length;
//   uint8_t action;
//   char data[MESSAGE_SIZE];
//   // uint8_t checksum;

// } Data_Frame;

// typedef struct _Messasge_Frame {
//     uint8_t frame_header; // 0xFE;
//     uint8_t data_length;
//     Data_Frame data_frame;
//     uint8_t checksum;
// } Message_Frame_Type;

// typedef union {
//     Message_Frame_Type item;
//     uint8_t raw[68];
// } Message_Frame;

/*Put your SSID & Password*/
// These are defined in the secrets.h (not provided)
const char *ssid = SECRET_SSID;    // Enter SSID here
const char *password = SECRET_KEY; // Enter Password here

const String index_html = "<!DOCTYPE html>\
<html>\
<head>\
<title>ZAM Textecke</title>\
<style>\
h1 {\
font-family: Arial, Helvetica, sans-serif;\
font-size: 32pt;\
text-align: center;\
background-color: #ffffff;\
color: rgb(12, 172, 172);\
}\
input.text_input {\
width: 95%%;\
}\
div.marquee {\
background-color: #000000;\
color: #FFFFFF;\
padding-left: 15px;\
padding-right: 15px;\
padding-top: 0px;\
padding-bottom: 0px;\
font-size: 32pt;\
}\
div.input_field {\
padding-top: 10px;\
padding-bottom: 10px;\
}\
.input_button {\
padding-left: 10px;\
padding-right: 10px;\
}\
div.input_form {\
font-family: Arial, Helvetica, sans-serif;\
padding: 15px;\
}\
p {\
-moz-animation: marquee 15s linear infinite;\
-webkit-animation: marquee 15s linear infinite;\
animation: marquee 15s linear infinite;\
padding-left: 15px;\
padding-right: 15px;\
}\
@-moz-keyframes marquee {\
0% {\
transform: translateX(100%);\
}\
\
100% {\
transform: translateX(-100%);\
}\
}\
@-webkit-keyframes marquee {\
0% {\
transform: translateX(100%);\
}\
\
100% {\
transform: translateX(-100%);\
}\
}\
@keyframes marquee {\
0% {\
-moz-transform: translateX(100%);\
-webkit-transform: translateX(100%);\
transform: translateX(100%)\
}\
\
100% {\
-moz-transform: translateX(-100%);\
-webkit-transform: translateX(-100%);\
transform: translateX(-100%);\
}\
}\
</style>\
</head>\
<body>\
<div class=\"header\">\
<h1>ZAM Textecke</h1>\
</div>\
<div class=\"marquee\">\
<p>{MESAGE_TOKEN}</p>\
</div>\
<div class=\"input_form\">\
<form id=\"message-form\" method=\"POST\" action=\"/textecke\">\
<fieldset>\
<legend>Nachrict</legend>\
<div class=\"input_field\">\
<label for=\"message-input\">Text eingeben:</label>\
<input id=\"message-input\" name=\"message\" placeholder=\"Enter some text\" maxlength=\"200\"\
class=\"text_input\" />\
</div>\
<div class=\"input_field\">\
<label for=\"background-select\">Hintergrund:</label>\
<select id=\"background-select\" name=\"background\">\
<option value=\"BLACK\" selected>black</option>\
<option value=\"RED\">red</option>\
<option value=\"ORANGE\">orange</option>\
<option value=\"YELLOW\">yellow</option>\
<option value=\"GREEN\">green</option>\
<option value=\"BLUE\">blue</option>\
<option value=\"VIOLET\">violet</option>\
<option value=\"WHITE\">white</option>\
</select>\
</div>\
<div class=\"input_field\">\
<label for=\"forground-select\">Voredergrund:</label>\
<select id=\"foreground-select\" name=\"foreground\">\
<option value=\"BLACK\">black</option>\
<option value=\"RED\">red</option>\
<option value=\"ORANGE\">orange</option>\
<option value=\"YELLOW\">yellow</option>\
<option value=\"GREEN\">green</option>\
<option value=\"BLUE\">blue</option>\
<option value=\"VIOLET\">violet</option>\
<option value=\"WHITE\" selected>white</option>\
</select>\
</div>\
</fieldset>\
<div class=\"input_field\">\
<button type=\"reset\" value=\"DEFAULT\" class=\"input_button\">zurucksetzen</button>\
<button value=\"OK\" class=\"input_button\">OK</button>\
</div>\
</form>\
</div>\
</body>\
</html>";

String html_page;
char current_message[200] = "empty...";
char new_message[200] = "empty...";
char foreground[16] = "WHITE";
char background[16] = "BLACK";

StaticJsonDocument<1024> doc;

ESP8266WebServer server(80);

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
  Serial.printf("check-sum: %02X\n", checksum);
  return checksum;
}


uint8_t get_colour_byte(String colour_name){
  uint8_t colour_byte = -1;
  if ( colour_name.equals("BLACK") == 0 ){
    colour_byte = 0x00;
  }
  if ( colour_name.equals("RED") == 0 ){
    colour_byte = 0x01;
  }
  if ( colour_name.equals("ORANGE") == 0 ){
    colour_byte = 0x02;
  }
  if ( colour_name.equals("YELLO") == 0 ){
    colour_byte = 0x03;
  }
  if ( colour_name.equals("GREEN") == 0 ){
    colour_byte = 0x04;
  }
  if ( colour_name.equals("BLUE") == 0 ){
    colour_byte = 0x05;
  }
  if ( colour_name.equals("INDIGO") == 0 ){
    colour_byte = 0x06;
  }
  if ( colour_name.equals("VIOLET") == 0 ){
    colour_byte = 0x07;
  }
  if ( colour_name.equals("WHITE") == 0 ){
    colour_byte = 0x08;
  }
  return colour_byte;
}

void render_and_send(String action, String param) {
    Serial.printf("action: %s, param: %s", action.c_str(), param.c_str());
    uint8_t message_frame[255];
    uint8_t frame_idx = 0;
    uint8_t frame_length = 0;
    message_frame[frame_idx++] = FRAME_DELIMITER;
    if (action.equals("message") == 0)
    {
      frame_length = 1; // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_MESSAGE;
      Serial.print("Receiving: ");
      bool escape_mode = false;
      for (uint8_t i = 0; i < param.length(); i++)
      {
        Serial.printf("%02x ", param[i]);
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
      Serial.println("");
      message_frame[0x01] = frame_length;
      Serial.printf("Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else if (action.equals("colour") == 0)
    {
      // Split the string around ":"
      // LHS=foreground, RHS=background.
      // The RHS is optional.
      //
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
      Serial.print("Receiving: ");

      message_frame[frame_idx++] = get_colour_byte(foreground);
      frame_length++;
      if (strlen(background)>0){
        message_frame[frame_idx++] = get_colour_byte(background);
        frame_length++;
      }
      Serial.println("");
      message_frame[0x01] = frame_length;
      Serial.printf("Setting up the datagram. Data length: %02X\n", frame_length);
    }
    else
    {
      Serial.print("The action does not match message");
      server.send(405, "text/plain", "Unknown or unsupported action");
      return;
    }

    uint8_t checksum = calc_checksum(&message_frame[2], frame_length);
    message_frame[frame_idx++] = checksum;
    for (uint8_t i = 0; i < frame_length + 3; i++)
    {
      Serial.printf("%02X ", message_frame[i]);
    }
    Serial.println("");
    Serial1.write(message_frame, frame_length + 3);
  
}

void handle_get_page_request()
{
  Serial.println("Received HTTP GET textecke request");
  if (server.method() != HTTP_GET)
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
  else
  {
    Serial.println("Starting form rendering");
    //sprintf(html_page.c_str(), index_html.c_str(), current_message);
    html_page = String(index_html);
    html_page.replace("{MESAGE_TOKEN}", current_message);
    // Serial.printf("PAGE: %s\n", html_page.c_str());
    server.send(200, "text/html", html_page);
  }
}

void handle_post_form_request()
{
  Serial.println("Received HTTP POST textecke request");
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    Serial.println("Starting form processing");
    String tempstr;
    Serial.println("Assigning the message");
    tempstr = server.arg("message");
    strcpy(new_message, tempstr.c_str());
    Serial.println("Assigning the foreground");
    tempstr = server.arg("foreground");
    strcpy(foreground, tempstr.c_str());
    Serial.println("Assigning the background");
    tempstr = server.arg("background");
    strcpy(background, tempstr.c_str());

    Serial.printf("message: %s\n", new_message);
    Serial.printf("foreground: %s\n", foreground);
    Serial.printf("background: %s\n", background);
    Serial.printf("Rendering the message to send %s\n", new_message);
    // render_and_send("message", new_message);

    // tempstr = String(foreground);
    // tempstr.concat(":");
    // tempstr.concat(background);

    // Serial.printf("Rendering the colours to send %s\n", tempstr.c_str());
    // render_and_send("colour", tempstr.c_str());

    strcpy(current_message, new_message);
    Serial.printf("Current Message %s\n", current_message);

    html_page = String(index_html);
    html_page.replace("{MESAGE_TOKEN}", current_message);
    // Serial.printf("Rendered HTML %s", html_page.c_str());
    server.send(200, "text/html", html_page);
  }
}

void handle_rest_request()
{
  Serial.println("Received REST POST request");

  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
  else
  {
    String result = "";

    if (server.hasArg("plain") == false)
    { // Check if body received
      server.send(200, "text/plain", "Body not received");
      return;
    }
    String content = server.arg("plain");
    Serial.println("Content: " + content);
    DeserializationError error = deserializeJson(doc, content);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      server.send(405, "text/plain", "bad content");
      return;
    }

    String action = doc["action"];
    String param = doc["param"];

    render_and_send(action, param);

    server.send(200, "text/plain; charset=utf-8", "OK");
  }
}


void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial1.begin(9600);
  delay(100);

  Serial.println("Connecting to ");
  Serial.println(ssid);
  // connect to your local wi-fi network
  WiFi.begin(ssid, password);

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  server.on("/api/v1/", HTTP_POST, handle_rest_request);
  server.on("/textecke", HTTP_GET, handle_get_page_request);
  server.on("/textecke", HTTP_POST, handle_post_form_request);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}
void loop()
{
  server.handleClient();
}
