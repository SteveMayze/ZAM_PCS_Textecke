#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

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
//const char* ssid = "SSID";  // Enter SSID here
//const char* password = "PWD";  //Enter Password here

StaticJsonDocument<1024> doc;

ESP8266WebServer server(80);

String send_response(){
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


uint8_t calc_checksum(uint8_t message[], uint8_t length){
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++){
      checksum += message[i];
  }
  checksum &= 0xFF;
  checksum =  0xFF - checksum;
  Serial.printf("check-sum: %02X\n", checksum);
  return checksum;
}

void handle_request() {

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String result = "";

    if (server.hasArg("plain")== false){ //Check if body received    
        server.send(200, "text/plain", "Body not received");
        return;
    }
    String content = server.arg("plain");
    Serial.println("Content: " + content);
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      server.send(405, "text/plain", "bad content");
      return;
    }

    string action = doc["action"];
    string param = doc["param"];
    Serial.printf("action: %s, param: %s", action.c_str(), param.c_str());
    uint8_t message_frame[255];
    uint8_t frame_idx = 0;
    uint8_t frame_length = 0;
    message_frame[frame_idx++] = FRAME_DELIMITER;
    if (action.compare("message") == 0){
      frame_length = 1;  // The length of the message + the action, one byte.
      message_frame[frame_idx++] = frame_length;
      message_frame[frame_idx++] = ACTION_MESSAGE;
      Serial.print("Receving: ");
      bool escape_mode = false;
      for(uint8_t i = 0; i<  param.length();i++){
        Serial.printf("%02x ", param[i]);
        if (escape_mode) {
          uint8_t subst = 0x20;
          for (uint8_t j = 0; j < SPECIAL_CHAR; j++){
            if ( charmap[j][0] == param[i]) {
              subst = charmap[j][1];
              break;
            }
          }
          message_frame[frame_idx++] = subst;
          frame_length++;
          escape_mode = false;
        } else if(param[i] == 0xC3 || param[i] == 0xC2 ){
          escape_mode = true;
        } else {
          message_frame[frame_idx++] = param[i];
          frame_length++;
        }
      } 
      Serial.println("");
      message_frame[0x01] = frame_length;
      Serial.printf("Setting up the datagram. Data length: %02X\n", frame_length);
    } else if (action.compare("colour") == 0){
      // Split the string around ":"
      // LHS=foreground, RHS=background.
      // The RHS is optional.
      //
    } else {
      Serial.print("The action does not match message");
      server.send(405, "text/plain", "Unknown or unsupported action");
      return;
    }

    uint8_t checksum =  calc_checksum(&message_frame[2], frame_length);
    message_frame[frame_idx++]  = checksum;
    for(uint8_t i = 0; i<frame_length+3; i++){
      Serial.printf("%02X ", message_frame[i]);
    }
    Serial.println("");
    Serial1.write(message_frame, frame_length+3);


    server.send(200, "text/plain; charset=utf-8", "OK");
  }
}


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial1.begin(9600);
  delay(100);

  Serial.println("Connecting to ");
  Serial.println(ssid);
  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", HTTP_POST, handle_request);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
}

